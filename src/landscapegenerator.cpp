#include "landscapegenerator.h"

#include <QPoint>
#include <QQueue>
#include <QSet>

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

    // Четыре платформы создают узнаваемый ландшафт. На каждой генерация
    // случайно выбирает разрыв и его ширину, оставляя вертикальный путь между
    // уровнями платформ. Разрывы делают карты разными, но не превращают их
    // в сплошные стены, отрезающие игрока от верхних областей.
    const int platformRows[] = {2, 4, 6, 8};
    QVector<QPoint> supportedCells;
    for (int platformRow : platformRows) {
        const int gapStart = 4 + random.bounded(10); // столбцы 4..13
        const int gapWidth = 2 + random.bounded(3);  // ширина 2..4 клетки

        for (int col = 2; col <= 17; ++col) {
            const bool insideGap = col >= gapStart && col < gapStart + gapWidth;
            if (!insideGap) {
                map[platformRow][col] = QLatin1Char('#');
                // Оставляем один столбец запаса по краям: к предмету можно
                // подойти с обеих сторон, не упираясь сразу в боковую стену.
                if (col > 2 && col < 17)
                    supportedCells.append(QPoint(col, platformRow - 1));
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

    // Добавляем кристаллы на поддержанные клетки. Они остаются препятствиями,
    // а не наградами; их клетки не используются как проходы или точки старта.
    // Повторный выбор пропускается, если там уже стоит обычный Приз.
    int crystalsPlaced = 0;
    for (int attempt = 0; attempt < 40 && crystalsPlaced < 3; ++attempt) {
        const QPoint cell = supportedCells[random.bounded(supportedCells.size())];
        if (map[cell.y()][cell.x()] != QLatin1Char('.'))
            continue;
        map[cell.y()][cell.x()] = QLatin1Char('C');
        ++crystalsPlaced;
    }

    // Суперприз появляется с вероятностью 1/4. Сначала случайно решаем,
    // создавать ли его, затем выбираем свободное место на поддержанной
    // поверхности. Seed управляет обоими решениями, поэтому результат
    // можно повторить и передать другому разработчику для разбора карты.
    if (random.bounded(4) == 0) {
        QVector<QPoint> freeSupportedCells;
        for (const QPoint &cell : supportedCells) {
            if (map[cell.y()][cell.x()] == QLatin1Char('.'))
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
            } else if (tile == QLatin1Char('S')) {
                ++superPrizeCount;
            } else if (tile == QLatin1Char('X')) {
                ++exitCount;
            }

            // Кристаллы, обычные Призы и Суперприз — подвижные сущности.
            // На старте под каждым из них должна находиться кирпичная опора,
            // иначе первый шаг физики может изменить карту ещё до решения игрока.
            if (tile == QLatin1Char('C') || tile == QLatin1Char('$')
                    || tile == QLatin1Char('S')) {
                if (row + 1 >= kRows || map[row + 1][col] != QLatin1Char('#'))
                    return fail(QStringLiteral("подвижный объект не имеет кирпичной опоры"));
            }
        }
    }

    if (playerCount != 1)
        return fail(QStringLiteral("на карте должна быть ровно одна стартовая позиция"));
    if (prizeCount < 1)
        return fail(QStringLiteral("на карте нет обязательных Призов"));
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

            if (tile == QLatin1Char('C')) {
                // Кристалл не собирается, но если он поставлен как толкаемое
                // препятствие, у игрока должны быть доступная сторона подхода
                // и свободная клетка назначения хотя бы в одном направлении.
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
        if (validateCandidate(candidate, &reason)) {
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
