#include "landscapegenerator.h"

#include <QPoint>
#include <QQueue>
#include <QSet>
#include <functional>

namespace {

constexpr int kColumns = 20;
constexpr int kRows = 13;

// Небольшой генератор псевдослучайных чисел с явно заданным состоянием.
// В отличие от глобального генератора Qt он не зависит от случайных вызовов
// в других частях программы: одинаковый seed даёт ту же последовательность.
// Xorshift выбран для учебного проекта из-за короткой и понятной реализации;
// для криптографии такой алгоритм использовать нельзя.
class SeededRandom
{
public:
    explicit SeededRandom(quint32 seed)
        : m_state(seed == 0 ? 0x6D2B79F5u : seed)
    {
        // У xorshift нулевое состояние навсегда остаётся нулевым, поэтому
        // seed 0 заменяется фиксированной ненулевой константой.
    }

    quint32 next()
    {
        // Три побитовых шага обновляют состояние и возвращают новое число.
        // Все операции выполняются над беззнаковым типом фиксированного размера.
        m_state ^= m_state << 13;
        m_state ^= m_state >> 17;
        m_state ^= m_state << 5;
        return m_state;
    }

    int bounded(int upperExclusive)
    {
        // Для небольших игровых диапазонов остаток от деления даёт простой
        // выбор индекса: результат всегда лежит в [0, upperExclusive).
        return upperExclusive > 0
                ? static_cast<int>(next() % static_cast<quint32>(upperExclusive))
                : 0;
    }

private:
    quint32 m_state;
};

bool isInside(const QVector<QString> &map, int row, int col)
{
    return row >= 0 && row < map.size()
            && col >= 0 && col < map[row].size();
}

// Поиск в ширину отвечает на практический вопрос: может ли игрок пройти
// от старта до клетки, если кирпичи и кристаллы пока считать препятствиями.
// Отдельный маршрут через копание проверяется ниже по специально заложенной
// геометрии головоломки — так проверка не притворяется полной физической симуляцией.
int cellKey(QPoint cell)
{
    // Одно целое число кодирует координату клетки: row * width + column.
    // Для карты фиксированного размера этого достаточно и удобно для QSet.
    return cell.y() * kColumns + cell.x();
}

QSet<int> reachableCells(const QVector<QString> &map, QPoint start)
{
    QSet<int> visited;
    QQueue<QPoint> queue;
    visited.insert(cellKey(start));
    queue.enqueue(start);

    const QPoint directions[] = {
        QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1)
    };

    while (!queue.isEmpty()) {
        const QPoint current = queue.dequeue();
        for (const QPoint &direction : directions) {
            const QPoint next = current + direction;
            if (!isInside(map, next.y(), next.x())
                    || visited.contains(cellKey(next)))
                continue;

            const QChar tile = map[next.y()][next.x()];
            if (tile == QLatin1Char('#') || tile == QLatin1Char('C'))
                continue;

            visited.insert(cellKey(next));
            queue.enqueue(next);
        }
    }
    return visited;
}

QVector<QString> createCandidate(quint32 seed)
{
    SeededRandom random(seed);
    QVector<QString> map(kRows, QString(kColumns, QLatin1Char('.')));

    // Сначала строим несущий каркас: замкнутый периметр, два портала,
    // стартовую комнату и нижнюю комнату с обязательным копанием.
    // Предметы и случайный рельеф добавляются только после этого шага.
    for (int row = 0; row < kRows; ++row) {
        map[row][0] = QLatin1Char('#');
        map[row][kColumns - 1] = QLatin1Char('#');
    }
    map[0] = QString(kColumns, QLatin1Char('#'));
    map[kRows - 1] = QString(kColumns, QLatin1Char('#'));

    map[11][1] = QLatin1Char('X'); // портал назад к предыдущему экрану
    map[11][2] = QLatin1Char('x'); // защищённая клетка рядом с порталом
    map[11][3] = QLatin1Char('P'); // старт в открытой нижней комнате
    map[11][17] = QLatin1Char('x');
    map[11][18] = QLatin1Char('X'); // второй безопасный выход

    // Эти кирпичи образуют карман с Призом. Клетка (10, 6) остаётся
    // доступной: если встать на неё и копать вправо, будет разрушен кирпич
    // (11, 7), после чего можно пройти к Призу в (11, 8). Кирпич справа
    // не даёт обойти награду через соседнюю клетку.
    map[10][7] = QLatin1Char('#');
    map[10][8] = QLatin1Char('#');
    map[10][9] = QLatin1Char('#');
    map[11][7] = QLatin1Char('#');
    map[11][8] = QLatin1Char('$');
    map[11][9] = QLatin1Char('#');

    // Четыре платформы создают основной рельеф. На каждой генерация случайно
    // выбирает узкий вертикальный проход шириной 1–2 клетки. Такие проходы
    // сложнее широких окон, но расположение смещается между платформами, чтобы
    // игроку приходилось искать маршрут по диагонали через несколько рядов.
    // После размещения Кристаллов добавляются безопасные подходы к ним.
    const int platformRows[] = {2, 4, 6, 8};
    int gapStarts[4] = {};
    int gapWidths[4] = {};
    QVector<QPoint> supportedCells;
    for (int platformIndex = 0; platformIndex < 4; ++platformIndex) {
        const int platformRow = platformRows[platformIndex];
        const int gapStart = 4 + random.bounded(11); // столбцы 4..14
        const int gapWidth = 1 + random.bounded(2);  // ширина 1–2 клетки
        gapStarts[platformIndex] = gapStart;
        gapWidths[platformIndex] = gapWidth;

        for (int col = 2; col <= 17; ++col) {
            const bool insideGap = col >= gapStart && col < gapStart + gapWidth;
            if (!insideGap) {
                map[platformRow][col] = QLatin1Char('#');
                // Оставляем один столбец запаса по краям: к предмету можно
                // подойти с обеих сторон, не упираясь сразу в боковую стену.
                // Клетки непосредственно у края шахты остаются пустыми:
                // это боковые «лазейки», через которые можно войти на полку.
                const bool nextToShaft = col == gapStart - 1
                        || col == gapStart + gapWidth;
                if (col > 2 && col < 17 && !nextToShaft) {
                    supportedCells.append(QPoint(col, platformRow - 1));
                }
            }
        }

        // Каждая поверхность получает обычный Приз над кирпичной опорой.
        // Позиции выбираются только на клетках платформы, а не над разрывом.
        QVector<int> platformColumns;
        for (int col = 2; col <= 17; ++col) {
            const bool insideGap = col >= gapStart && col < gapStart + gapWidth;
            if (!insideGap)
                platformColumns.append(col);
        }
        const int prizeCol = platformColumns[random.bounded(platformColumns.size())];
        map[platformRow - 1][prizeCol] = QLatin1Char('$');
    }

    // Короткие вертикальные простенки под первыми тремя платформами превращают
    // плоские полки в череду ниш. Основные шахты не застраиваем: в их координатах
    // проход должен оставаться открытым. Последний горизонтальный ряд (9)
    // намеренно оставлен свободным как связующий маршрут к правому выходу.
    for (int platformIndex = 0; platformIndex < 3; ++platformIndex) {
        const int supportRow = platformRows[platformIndex] + 1;
        const int pillarCount = 2 + random.bounded(2); // 2–3 простенка
        int pillarsPlaced = 0;
        for (int attempt = 0; attempt < 32 && pillarsPlaced < pillarCount; ++attempt) {
            const int col = 2 + random.bounded(16); // внутренние столбцы 2..17
            const bool insideShaft = col >= gapStarts[platformIndex]
                    && col < gapStarts[platformIndex] + gapWidths[platformIndex];
            if (insideShaft || map[supportRow][col] != QLatin1Char('.'))
                continue;
            map[supportRow][col] = QLatin1Char('#');
            ++pillarsPlaced;
        }
    }

    // Выбираем пять разнесённых Кристаллов на каждой платформе.
    // Случайное плотное размещение создавало цепочки, которые валидатор
    // справедливо отклонял: к ним нельзя подойти с пустой стороны.
    for (int platformRow : platformRows) {
        const int row = platformRow - 1;
        QVector<int> eligible;
        for (int col = 2; col <= 17; ++col) {
            if (map[platformRow][col] == QLatin1Char('#')
                    && (map[row][col] == QLatin1Char('.') || map[row][col] == QLatin1Char('#'))
                    && map[row][col - 1] != QLatin1Char('$')
                    && map[row][col + 1] != QLatin1Char('$'))
                eligible.append(col);
        }
        QVector<QVector<int>> choices;
        QVector<int> selected;
        std::function<void(int)> choose = [&](int index) {
            if (selected.size() == 5) {
                choices.append(selected);
                return;
            }
            for (int i = index; i < eligible.size(); ++i) {
                if (!selected.isEmpty() && eligible[i] - selected.last() < 2)
                    continue;
                selected.append(eligible[i]);
                choose(i + 1);
                selected.removeLast();
            }
        };
        choose(0);
        if (choices.isEmpty())
            return map; // Общий валидатор отклонит неполный набор.
        const auto columns = choices[random.bounded(choices.size())];
        for (int col : columns) {
            map[row][col] = QLatin1Char('C');
            for (int side : {col - 1, col + 1}) {
                if (map[row][side] == QLatin1Char('#'))
                    map[row][side] = QLatin1Char('.');
            }
        }
    }

    // Подход к Кристаллам должен быть доступен и над верхней платформой:
    // одного общего прохода недостаточно для пяти разделяющих её объектов.
    // Открываем клетки под пустыми соседями, сохраняя опоры всех предметов.
    for (int platformRow : platformRows) {
        const int row = platformRow - 1;
        for (int col = 2; col <= 17; ++col) {
            if (map[row][col] != QLatin1Char('C'))
                continue;
            for (int side : {col - 1, col + 1}) {
                if (map[row][side] != QLatin1Char('.'))
                    continue;
                map[platformRow][side] = QLatin1Char('.');
                if (map[platformRow + 1][side] == QLatin1Char('#'))
                    map[platformRow + 1][side] = QLatin1Char('.');
            }
        }
    }

    // Суперприз появляется с вероятностью 1/4. Сначала случайно решаем,
    // создавать ли его, затем выбираем свободное место на поддержанной
    // поверхности. Seed управляет обоими решениями, поэтому результат
    // можно повторить и передать другому разработчику для разбора карты.
    if (random.bounded(4) == 0) {
        QVector<QPoint> freeSupportedCells;
        for (const QPoint &cell : supportedCells) {
            if (map[cell.y()][cell.x()] == QLatin1Char('.')
                    && map[cell.y() + 1][cell.x()] == QLatin1Char('#'))
                freeSupportedCells.append(cell);
        }
        if (!freeSupportedCells.isEmpty()) {
            const QPoint superPrize = freeSupportedCells[
                    random.bounded(freeSupportedCells.size())];
            map[superPrize.y()][superPrize.x()] = QLatin1Char('S');
        }
    }

    return map;
}

bool validateCandidate(const QVector<QString> &map, QString *reason)
{
    const auto fail = [reason](const QString &message) {
        if (reason)
            *reason = message;
        return false;
    };

    // Сначала проверяем форму данных, чтобы дальнейшие обращения к клеткам
    // не могли выйти за границы строк или принять повреждённую карту.
    if (map.size() != kRows)
        return fail(QStringLiteral("неверное количество строк"));
    for (const QString &row : map) {
        if (row.size() != kColumns)
            return fail(QStringLiteral("строки карты имеют разную ширину"));
    }
    for (int col = 0; col < kColumns; ++col) {
        if (map.first()[col] != QLatin1Char('#')
                || map.last()[col] != QLatin1Char('#'))
            return fail(QStringLiteral("верхняя или нижняя граница открыта"));
    }
    for (int row = 0; row < kRows; ++row) {
        if (map[row][0] != QLatin1Char('#')
                || map[row][kColumns - 1] != QLatin1Char('#'))
            return fail(QStringLiteral("боковая граница открыта"));
    }

    int playerCount = 0;
    int prizeCount = 0;
    int crystalCount = 0;
    int superPrizeCount = 0;
    int exitCount = 0;
    QPoint start;
    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kColumns; ++col) {
            const QChar tile = map[row][col];
            if (tile == QLatin1Char('P')) {
                ++playerCount;
                start = QPoint(col, row);
            } else if (tile == QLatin1Char('$')) {
                ++prizeCount;
            } else if (tile == QLatin1Char('C')) {
                ++crystalCount;
            } else if (tile == QLatin1Char('S')) {
                ++superPrizeCount;
            } else if (tile == QLatin1Char('X')) {
                ++exitCount;
            }

            // Верхние Кристаллы колонны опираются на нижние. Призы и
            // Суперприз по-прежнему требуют собственной кирпичной опоры.
            if (tile == QLatin1Char('C') || tile == QLatin1Char('$')
                    || tile == QLatin1Char('S')) {
                if (row + 1 >= kRows || (map[row + 1][col] != QLatin1Char('#')
                        && !(tile == QLatin1Char('C')
                             && map[row + 1][col] == QLatin1Char('C'))))
                    return fail(QStringLiteral("подвижный объект не имеет опоры"));
                if (tile == QLatin1Char('C') && map[row + 1][col] == QLatin1Char('C')) {
                    for (int direction : {-1, 1}) {
                        if (map[row][col + direction] == QLatin1Char('.')
                                && map[row + 1][col + direction] == QLatin1Char('.'))
                            return fail(QStringLiteral("колонна Кристаллов осыпается на старте"));
                    }
                }
            }
        }
    }

    if (playerCount != 1)
        return fail(QStringLiteral("на карте должна быть ровно одна стартовая позиция"));
    if (prizeCount < 1)
        return fail(QStringLiteral("на карте нет обязательных Призов"));
    if (crystalCount != 20)
        return fail(QStringLiteral("на карте должно быть ровно 20 Кристаллов"));
    if (superPrizeCount > 1)
        return fail(QStringLiteral("на карте больше одного Суперприза"));
    if (exitCount < 1)
        return fail(QStringLiteral("на карте нет выхода"));

    // Персонаж должен иметь безопасный первый ход. В этой карте гарантирован
    // проход вверх из старта; проверяем его явно, чтобы изменение каркаса
    // не заперло игрока незаметно для автора генератора.
    if (map[start.y() - 1][start.x()] != QLatin1Char('.'))
        return fail(QStringLiteral("стартовая клетка не имеет свободного первого хода"));

    const QSet<int> reachable = reachableCells(map, start);
    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kColumns; ++col) {
            const QChar tile = map[row][col];
            if ((tile == QLatin1Char('$') || tile == QLatin1Char('S'))
                    && row == 11 && col == 8) {
                // Эта награда намеренно закрыта кирпичом: вместо обычной
                // достижимости проверяем безопасную клетку для копания ниже.
                continue;
            }
            if ((tile == QLatin1Char('$') || tile == QLatin1Char('S'))
                    && !reachable.contains(row * kColumns + col))
                return fail(QStringLiteral("обычный Приз или Суперприз недостижим"));

            if (tile == QLatin1Char('C') && map[row + 1][col] != QLatin1Char('C')) {
                // Для одиночного C или основания колонны нужны доступная
                // сторона подхода и пустая клетка назначения. Верхние C
                // освобождаются падением после сдвига основания.
                bool canPush = false;
                for (int direction : {-1, 1}) {
                    const int approachCol = col - direction;
                    const int destinationCol = col + direction;
                    if (approachCol > 0 && destinationCol < kColumns - 1
                            && reachable.contains(row * kColumns + approachCol)
                            && map[row][destinationCol] == QLatin1Char('.')) {
                        canPush = true;
                        break;
                    }
                }
                if (!canPush)
                    return fail(QStringLiteral("кристалл нельзя безопасно подтолкнуть"));
            }
        }
    }

    // Проверяем учебную задачу «подойти — выкопать — пройти к награде».
    // Игрок может добраться до (10, 6), выкопать кирпич (11, 7), а затем
    // пройти к Призу (11, 8); опорный кирпич у награды не даёт ей упасть.
    if (!reachable.contains(10 * kColumns + 6)
            || map[11][7] != QLatin1Char('#')
            || map[11][8] != QLatin1Char('$')
            || map[11][9] != QLatin1Char('#'))
        return fail(QStringLiteral("не найден доступный маршрут копания к Призу"));

    // Оба портала должны быть достижимы. Выход справа проверяется на карте,
    // где боковые клетки x остаются проходимыми для Персонажа.
    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kColumns; ++col) {
            if (map[row][col] == QLatin1Char('X')
                    && !reachable.contains(row * kColumns + col))
                return fail(QStringLiteral("выход недостижим от старта"));
        }
    }

    return true;
}

int crystalColumnCount(const QVector<QString> &map)
{
    int count = 0;
    for (int row = 1; row < kRows - 1; ++row) {
        for (int col = 1; col < kColumns - 1; ++col) {
            if (map[row][col] != QLatin1Char('C') || map[row - 1][col] == QLatin1Char('C'))
                continue;
            int bottom = row;
            while (bottom + 1 < kRows && map[bottom + 1][col] == QLatin1Char('C'))
                ++bottom;
            if (bottom - row + 1 >= 3)
                ++count;
        }
    }
    return count;
}

bool addCrystalColumns(QVector<QString> &map, quint32 seed, QString *reason)
{
    SeededRandom random(seed ^ 0xC2B2AE35u);
    QVector<QPoint> bases;
    for (int row : {3, 5, 7})
        for (int col = 2; col <= 17; ++col)
            if (map[row][col] == QLatin1Char('C'))
                bases.append(QPoint(col, row));
    for (int index = bases.size() - 1; index > 0; --index)
        bases.swapItemsAt(index, random.bounded(index + 1));

    QVector<int> usedColumns;
    for (const QPoint &base : bases) {
        const int row = base.y();
        const int col = base.x();
        bool separate = true;
        for (int used : usedColumns)
            if (qAbs(used - col) < 3)
                separate = false;
        if (!separate || map[row][col] != QLatin1Char('C')
                || map[row - 1][col] != QLatin1Char('#')
                || (map[row - 2][col] != QLatin1Char('.')
                    && map[row - 2][col] != QLatin1Char('C')))
            continue;

        // Средний ярус закрыт с боков: верхние C не скатываются до того,
        // как игрок сдвинет основание. После сдвига два верхних C падают
        // в освобождённую клетку как настоящая вертикальная цепочка.
        if ((map[row - 1][col - 1] != QLatin1Char('.')
                && map[row - 1][col - 1] != QLatin1Char('#'))
                || (map[row - 1][col + 1] != QLatin1Char('.')
                    && map[row - 1][col + 1] != QLatin1Char('#')))
            continue;
        const int required = map[row - 2][col] == QLatin1Char('C') ? 1 : 2;
        QVector<QPoint> donors;
        for (int donorRow = 1; donorRow < kRows - 1; ++donorRow) {
            for (int donorCol = 1; donorCol < kColumns - 1; ++donorCol) {
                if (map[donorRow][donorCol] != QLatin1Char('C')
                        || map[donorRow - 1][donorCol] == QLatin1Char('C')
                        || map[donorRow + 1][donorCol] == QLatin1Char('C')
                        || (donorCol == col && donorRow >= row - 2 && donorRow <= row))
                    continue;
                donors.append(QPoint(donorCol, donorRow));
            }
        }
        for (int index = donors.size() - 1; index > 0; --index)
            donors.swapItemsAt(index, random.bounded(index + 1));
        // Разные доноры открывают разные подходы к соседним полкам.
        // Перебор ограничен четырьмя вариантами на основание.
        for (int attempt = 0; attempt < 4 && donors.size() >= required; ++attempt) {
            auto candidate = map;
            for (int index = 0; index < required; ++index) {
                const QPoint donor = donors[(attempt * required + index) % donors.size()];
                candidate[donor.y()][donor.x()] = QLatin1Char('.');
            }
            candidate[row - 2][col] = QLatin1Char('C');
            candidate[row - 1][col] = QLatin1Char('C');
            candidate[row - 1][col - 1] = QLatin1Char('#');
            candidate[row - 1][col + 1] = QLatin1Char('#');
            if (!validateCandidate(candidate, nullptr))
                continue;
            map = candidate;
            usedColumns.append(col);
            break;
        }
        if (crystalColumnCount(map) >= 3)
            return true;
    }
    if (reason)
        *reason = QStringLiteral("не удалось построить три доступные колонны Кристаллов");
    return false;
}

bool addTerrainPartitions(QVector<QString> &map, quint32 seed, QString *reason)
{
    SeededRandom random(seed ^ 0xA511E9B3u);
    const int targetBricks = 18 + random.bounded(7);
    int addedBricks = 0;

    // Нижняя стартовая строка остаётся свободной для порталов и кармана.
    // Первый шаг вверх и позицию для копания не застраиваем даже тогда,
    // когда статический поиск смог бы найти обходной путь.
    const auto protectedCell = [](int row, int col) {
        return row <= 0 || row >= kRows - 2 || col <= 0 || col >= kColumns - 1
                || (row == 10 && (col == 3 || col == 6));
    };
    const auto tryPartition = [&](const QVector<QPoint> &cells) {
        QVector<QPoint> emptyCells;
        for (const QPoint &cell : cells) {
            if (protectedCell(cell.y(), cell.x()))
                return false;
            const QChar tile = map[cell.y()][cell.x()];
            if (tile == QLatin1Char('.'))
                emptyCells.append(cell);
            else if (tile != QLatin1Char('#'))
                return false; // Предметы, старт и выходы не заменяются стеной.
        }
        if (emptyCells.isEmpty() || addedBricks + emptyCells.size() > targetBricks)
            return false;

        auto candidate = map;
        for (const QPoint &cell : emptyCells)
            candidate[cell.y()][cell.x()] = QLatin1Char('#');
        // Проверяем целую перегородку, а не отдельные декоративные клетки:
        // должны остаться Призы, выходы и хотя бы одна сторона толкания C.
        if (!validateCandidate(candidate, nullptr))
            return false;
        map = candidate;
        addedBricks += emptyCells.size();
        return true;
    };

    // Первая перегородка продолжает правую стенку кармана вверх до строки 8.
    // Прямого обхода по строке 9 больше нет: статический путь к правому
    // выходу проходит не ниже строки 7 (минимум 23 шага вместо прежних 19).
    if (!tryPartition({QPoint(10, 8), QPoint(10, 9), QPoint(10, 10)})) {
        if (reason)
            *reason = QStringLiteral("не удалось сохранить обход нижней перегородки");
        return false;
    }

    // В прежде пустой нижней правой комнате появляются дополнительные
    // вертикальные простенки. У порталов сохраняется свободная строка 11.
    QVector<int> lowerColumns{11, 12, 13, 14, 15, 16};
    for (int index = lowerColumns.size() - 1; index > 0; --index)
        lowerColumns.swapItemsAt(index, random.bounded(index + 1));
    int lowerPartitions = 1;
    for (int col : lowerColumns) {
        if (tryPartition({QPoint(col, 9), QPoint(col, 10)}))
            ++lowerPartitions;
        if (lowerPartitions == 3)
            break;
    }
    if (lowerPartitions < 2) {
        if (reason)
            *reason = QStringLiteral("не удалось построить нижние перегородки");
        return false;
    }

    // Продлеваем опоры и создаём короткие стены и угловые ниши в разных
    // частях карты. Перестановка набора вариантов воспроизводима по seed.
    const QVector<QPoint> shapes[] = {
        {QPoint(0, 0), QPoint(0, 1)},
        {QPoint(0, 0), QPoint(1, 0)},
        {QPoint(0, 0), QPoint(0, 1), QPoint(0, 2)},
        {QPoint(0, 0), QPoint(1, 0), QPoint(2, 0)},
        {QPoint(0, 0), QPoint(0, 1), QPoint(1, 1)},
        {QPoint(0, 0), QPoint(1, 0), QPoint(1, 1)},
        {QPoint(0, 0), QPoint(0, 1), QPoint(-1, 1)}
    };
    QVector<QVector<QPoint>> partitions;
    for (int row = 2; row <= 10; ++row) {
        for (int col = 1; col <= 18; ++col) {
            for (const auto &shape : shapes) {
                QVector<QPoint> cells;
                bool allowed = true;
                for (const QPoint &offset : shape) {
                    const QPoint cell = QPoint(col, row) + offset;
                    if (protectedCell(cell.y(), cell.x())
                            || (map[cell.y()][cell.x()] != QLatin1Char('.')
                                && map[cell.y()][cell.x()] != QLatin1Char('#'))) {
                        allowed = false;
                        break;
                    }
                    cells.append(cell);
                }
                if (allowed)
                    partitions.append(cells);
            }
        }
    }
    for (int index = partitions.size() - 1; index > 0; --index)
        partitions.swapItemsAt(index, random.bounded(index + 1));
    for (const auto &cells : partitions) {
        tryPartition(cells);
        if (addedBricks == targetBricks)
            break;
    }

    // Одиночные продолжения позволяют закончить стены там, где целая группа
    // перекрыла бы единственный маршрут. Число вариантов ограничено полем.
    QVector<QPoint> remainingCells;
    for (int row = 2; row <= 10; ++row)
        for (int col = 1; col <= 18; ++col)
            if (!protectedCell(row, col) && map[row][col] == QLatin1Char('.'))
                remainingCells.append(QPoint(col, row));
    for (int index = remainingCells.size() - 1; index > 0; --index)
        remainingCells.swapItemsAt(index, random.bounded(index + 1));
    for (const QPoint &cell : remainingCells) {
        tryPartition({cell});
        if (addedBricks == targetBricks)
            break;
    }
    if (addedBricks < 12) {
        if (reason)
            *reason = QStringLiteral("недостаточно безопасных дополнительных перегородок");
        return false;
    }
    return true;
}

} // namespace

QVector<QString> LandscapeGenerator::generate(quint32 seed, QString *error)
{
    // Каждая попытка остаётся воспроизводимой: номер попытки однозначно
    // меняет seed. Это пригодится, если будущие правила добавят больше
    // случайных препятствий и часть карт начнёт отклоняться проверкой.
    for (quint32 attempt = 0; attempt < 32; ++attempt) {
        const quint32 attemptSeed = seed + attempt * 0x9E3779B9u;
        QVector<QString> candidate = createCandidate(attemptSeed);
        QString reason;
        if (validateCandidate(candidate, &reason)
                && addCrystalColumns(candidate, attemptSeed, &reason)
                && addTerrainPartitions(candidate, attemptSeed, &reason)
                && crystalColumnCount(candidate) >= 3) {
            if (error)
                error->clear();
            return candidate;
        }
        if (error)
            *error = reason;
    }

    // Ошибка не маскируется пустой или частично проходимой картой: движок
    // сможет показать причину и оставить текущий уровень нетронутым.
    return {};
}
