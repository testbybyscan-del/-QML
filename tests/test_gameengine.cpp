#include "../src/gameengine.h"
#include "../src/landscapegenerator.h"

#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTimer>
#include <QPoint>
#include <QSet>
#include <QtTest>
#include <limits>
#include <algorithm>

namespace {

QStringList emptyLevel()
{
    return {
        QStringLiteral("#########"),
        QStringLiteral("#.......#"),
        QStringLiteral("#.......#"),
        QStringLiteral("#.P.....#"),
        QStringLiteral("#.......#"),
        QStringLiteral("#X.....X#"),
        QStringLiteral("#########")
    };
}

void setCell(QStringList &rows, int row, int column, QChar tile)
{
    QString line = rows.at(row);
    line[column] = tile;
    rows[row] = line;
}

QVariantList asVariantRows(const QStringList &rows)
{
    QVariantList result;
    for (const QString &row : rows)
        result.append(row);
    return result;
}

QStringList loadQmlLevel(QQmlEngine &qmlEngine, const QString &url)
{
    QQmlComponent component(&qmlEngine, QUrl(url));
    if (component.isError())
        return {};

    QObject *level = component.create();
    if (!level)
        return {};

    QStringList rows;
    for (const QVariant &row : level->property("rows").toList())
        rows.append(row.toString());
    delete level;
    return rows;
}

int countPrizes(const QStringList &rows)
{
    int count = 0;
    for (const QString &row : rows)
        count += row.count(QLatin1Char('$'));
    return count;
}

QVector<int> walkingDistances(const QVector<QString> &rows, QPoint start)
{
    const int width = rows.first().size();
    QVector<int> distances(rows.size() * width, -1);
    QVector<QPoint> pending{start};
    distances[start.y() * width + start.x()] = 0;
    const QPoint directions[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int index = 0; index < pending.size(); ++index) {
        const QPoint current = pending[index];
        for (const auto &direction : directions) {
            const QPoint next = current + direction;
            if (next.y() < 0 || next.y() >= rows.size()
                    || next.x() < 0 || next.x() >= width)
                continue;
            const int key = next.y() * width + next.x();
            if (distances[key] >= 0 || rows[next.y()][next.x()] == '#'
                    || rows[next.y()][next.x()] == 'C')
                continue;
            distances[key] = distances[current.y() * width + current.x()] + 1;
            pending.append(next);
        }
    }
    return distances;
}

QVector<quint32> generatedMapSeeds()
{
    QVector<quint32> seeds;
    for (quint32 seed = 1; seed <= 256; ++seed)
        seeds.append(seed);
    seeds.append(0);
    seeds.append(std::numeric_limits<quint32>::max());
    seeds.append(0x9E3779B9u);
    seeds.append(2026);
    return seeds;
}

QString tileTypeAt(GameEngine &game, int row, int column)
{
    TileModel *model = game.tilesModel();
    for (int index = 0; index < model->rowCount(); ++index) {
        const QModelIndex modelIndex = model->index(index);
        if (model->data(modelIndex, TileModel::RowRole).toInt() == row
                && model->data(modelIndex, TileModel::ColRole).toInt() == column)
            return model->data(modelIndex, TileModel::TypeRole).toString();
    }
    return {};
}

QSet<int> tileCells(GameEngine &game, const QString &type)
{
    QSet<int> cells;
    auto *model = game.tilesModel();
    for (int row = 0; row < model->rowCount(); ++row) {
        const auto index = model->index(row);
        if (model->data(index, TileModel::TypeRole).toString() == type)
            cells.insert(model->data(index, TileModel::RowRole).toInt() * game.columns()
                         + model->data(index, TileModel::ColRole).toInt());
    }
    return cells;
}

} // namespace

class GameEngineTest : public QObject
{
    Q_OBJECT

private:
    void initialize(GameEngine &game, const QStringList &rows)
    {
        QVariantList authoredLevels;
        authoredLevels.append(QVariant::fromValue(asVariantRows(rows)));
        game.initializeLevels(authoredLevels);
    }

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("BoulderDashTests"));
        QCoreApplication::setApplicationName(QStringLiteral("GameEngineTests"));
        QStandardPaths::setTestModeEnabled(true);
    }

    void generatedRouteIsReproducibleAndObjectsHaveSupports()
    {
        QSet<QString> differentMaps;
        QSet<QString> differentLandscapes;
        const auto seeds = generatedMapSeeds();
        for (quint32 seed : seeds) {
            QString error;
            const auto rows = LandscapeGenerator::generate(seed, &error);
            QVERIFY2(!rows.isEmpty(), qPrintable(QString("seed %1: %2").arg(seed).arg(error)));
            QCOMPARE(rows, LandscapeGenerator::generate(seed));
            QCOMPARE(rows.size(), 13);
            int crystals = 0;
            int players = 0;
            int prizes = 0;
            int exits = 0;
            int superPrizes = 0;
            int verticalColumns = 0;
            int columnCrystals = 0;
            QString serialized;
            QString landscape;
            for (int row = 0; row < rows.size(); ++row) {
                QCOMPARE(rows[row].size(), 20);
                serialized += rows[row] + '\n';
                for (int col = 0; col < rows[row].size(); ++col) {
                    const QChar tile = rows[row][col];
                    QVERIFY2(QStringLiteral("#.PXCx$S").contains(tile),
                             qPrintable(QString("seed %1: unsupported tile").arg(seed)));
                    landscape += tile == '#' ? '#' : '.';
                    if (row == 0 || row == 12 || col == 0 || col == 19)
                        QCOMPARE(tile, QChar('#'));
                    if (tile == 'C')
                        ++crystals;
                    players += tile == 'P';
                    prizes += tile == '$';
                    exits += tile == 'X';
                    superPrizes += tile == 'S';
                    if (tile == '$' || tile == 'S') {
                        QVERIFY(row + 1 < rows.size());
                        QCOMPARE(rows[row + 1][col], QChar('#'));
                    }
                    if (tile == 'C') {
                        QVERIFY(row + 1 < rows.size());
                        QVERIFY(rows[row + 1][col] == '#' || rows[row + 1][col] == 'C');
                        if (row == 0 || rows[row - 1][col] != 'C') {
                            int height = 1;
                            while (row + height < rows.size() && rows[row + height][col] == 'C')
                                ++height;
                            QCOMPARE(rows[row + height][col], QChar('#'));
                            if (height >= 3) {
                                ++verticalColumns;
                                columnCrystals += height;
                            }
                        }
                        if (rows[row + 1][col] == 'C') {
                            // Верхние Кристаллы стоят на цепочке; боковая
                            // геометрия должна удерживать её до действия игрока.
                            for (int direction : {-1, 1})
                                QVERIFY(!(rows[row][col + direction] == '.'
                                          && rows[row + 1][col + direction] == '.'));
                        }
                    }
                }
            }
            QCOMPARE(crystals, 20);
            QCOMPARE(players, 1);
            QCOMPARE(prizes, 5);
            QCOMPARE(exits, 2);
            QVERIFY(superPrizes <= 1);
            QVERIFY2(verticalColumns >= 3,
                     qPrintable(QString("seed %1: fewer than three tall crystal columns").arg(seed)));
            QVERIFY(columnCrystals >= 9);
            QCOMPARE(rows[11][1], QChar('X'));
            QCOMPARE(rows[11][2], QChar('x'));
            QCOMPARE(rows[11][3], QChar('P'));
            QCOMPARE(rows[10][3], QChar('.'));
            QCOMPARE(rows[11][17], QChar('x'));
            QCOMPARE(rows[11][18], QChar('X'));
            QCOMPARE(rows[11][7], QChar('#'));
            QCOMPARE(rows[11][8], QChar('$'));
            QCOMPARE(rows[11][9], QChar('#'));

            // Раньше правая нижняя комната была целиком открыта.
            // Теперь в ней обязательны хотя бы две вертикальные перегородки,
            // а не только случайное изменение позиции предметов на полках.
            int lowerRoomBricks = 0;
            int verticalPartitions = 0;
            for (int col = 10; col <= 16; ++col) {
                lowerRoomBricks += rows[9][col] == '#';
                lowerRoomBricks += rows[10][col] == '#';
                verticalPartitions += rows[9][col] == '#' && rows[10][col] == '#';
            }
            QVERIFY2(lowerRoomBricks >= 4,
                     qPrintable(QString("seed %1: lower room still lacks obstacles").arg(seed)));
            QVERIFY2(verticalPartitions >= 2,
                     qPrintable(QString("seed %1: lower room has no vertical partitions").arg(seed)));

            const auto distances = walkingDistances(rows, QPoint(3, 11));
            QVERIFY2(distances[11 * 20 + 1] >= 0 && distances[11 * 20 + 18] >= 0,
                     qPrintable(QString("seed %1: exits not connected to start").arg(seed)));
            QVERIFY2(distances[11 * 20 + 18] >= 23,
                     qPrintable(QString("seed %1: lower partition does not require a detour").arg(seed)));
            QVERIFY2(distances[10 * 20 + 6] >= 0,
                     qPrintable(QString("seed %1: digging approach is blocked").arg(seed)));
            for (int row = 1; row < 12; ++row) {
                for (int col = 1; col < 19; ++col) {
                    const QChar tile = rows[row][col];
                    if ((tile == '$' || tile == 'S') && !(row == 11 && col == 8))
                        QVERIFY2(distances[row * 20 + col] >= 0,
                                 qPrintable(QString("seed %1: unreachable reward at %2,%3")
                                                .arg(seed).arg(row).arg(col)));
                    if (tile == 'C' && rows[row + 1][col] != 'C') {
                        const bool pushLeft = distances[row * 20 + col + 1] >= 0
                                && rows[row][col - 1] == '.';
                        const bool pushRight = distances[row * 20 + col - 1] >= 0
                                && rows[row][col + 1] == '.';
                        QVERIFY2(pushLeft || pushRight,
                                 qPrintable(QString("seed %1: no push approach at %2,%3")
                                                .arg(seed).arg(row).arg(col)));
                    }
                }
            }
            differentMaps.insert(serialized);
            differentLandscapes.insert(landscape);
        }
        // Объекты и кирпичная геометрия должны меняться между seed:
        // одного фиксированного шаблона с переставленными наградами недостаточно.
        QVERIFY(differentMaps.size() >= seeds.size() / 2);
        QVERIFY(differentLandscapes.size() >= seeds.size() / 2);
    }

    void generatedCrystalColumnsFallAfterPushingBase_data()
    {
        QTest::addColumn<quint32>("seed");
        QTest::newRow("first-landscape") << quint32(1);
        QTest::newRow("landscape-at-level-50") << quint32(44);
        QTest::newRow("different-layout") << quint32(2026);
    }

    void generatedCrystalColumnsFallAfterPushingBase()
    {
        QFETCH(quint32, seed);
        QString error;
        const auto rows = LandscapeGenerator::generate(seed, &error);
        QVERIFY2(!rows.isEmpty(), qPrintable(error));
        GameEngine game;
        initialize(game, QStringList(rows.begin(), rows.end()));
        const auto initialCrystals = tileCells(game, "crystal");
        const auto initialGold = tileCells(game, "gold");
        game.updateCrystals();
        QCOMPARE(tileCells(game, "crystal"), initialCrystals);
        QCOMPARE(tileCells(game, "gold"), initialGold);
        QVERIFY(!game.gameOver());

        const QPoint start(game.playerX(), game.playerY());
        const auto distances = walkingDistances(rows, start);
        QPoint base(-1, -1);
        int direction = 0;
        for (int row = 3; row < rows.size() - 1 && direction == 0; ++row) {
            for (int col = 2; col < rows.first().size() - 2 && direction == 0; ++col) {
                if (rows[row][col] != 'C' || rows[row - 1][col] != 'C'
                        || rows[row - 2][col] != 'C' || rows[row + 1][col] != '#')
                    continue;
                for (int candidate : {-1, 1}) {
                    if (distances[row * game.columns() + col - candidate] >= 0
                            && rows[row][col + candidate] == '.') {
                        base = QPoint(col, row);
                        direction = candidate;
                        break;
                    }
                }
            }
        }
        QVERIFY2(direction != 0, "No generated column base can be reached and pushed");

        // Восстанавливаем кратчайший маршрут к основанию по независимому BFS.
        // Физика вызывается отдельно: это проверка причинного эффекта толчка,
        // а не симуляция полного прохождения уровня с таймерами.
        QVector<QPoint> route;
        QPoint cursor(base.x() - direction, base.y());
        const QPoint steps[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        while (cursor != start) {
            route.append(cursor);
            const int distance = distances[cursor.y() * game.columns() + cursor.x()];
            bool found = false;
            for (const auto &step : steps) {
                const QPoint previous = cursor + step;
                if (previous.x() > 0 && previous.x() < game.columns() - 1
                        && previous.y() > 0 && previous.y() < game.rows() - 1
                        && distances[previous.y() * game.columns() + previous.x()] == distance - 1) {
                    cursor = previous;
                    found = true;
                    break;
                }
            }
            QVERIFY(found);
        }
        std::reverse(route.begin(), route.end());
        for (const auto &point : route) {
            game.movePlayer(point.x() - game.playerX(), point.y() - game.playerY());
            QCOMPARE(QPoint(game.playerX(), game.playerY()), point);
            QVERIFY(!game.gameOver());
        }
        const auto goldBeforePush = tileCells(game, "gold");
        const int prizesBeforePush = game.prizesLeft();
        const int scoreBeforePush = game.score();
        game.movePlayer(direction, 0);
        QCOMPARE(QPoint(game.playerX(), game.playerY()), base);
        QCOMPARE(tileTypeAt(game, base.y(), base.x()), QString("empty"));
        QCOMPARE(tileTypeAt(game, base.y(), base.x() + direction), QString("crystal"));
        game.movePlayer(-direction, 0); // освободить клетку под верхней парой
        for (int tick = 0; tick < 32
                && tileTypeAt(game, base.y() - 2, base.x()) == "crystal"; ++tick)
            game.updateCrystals();

        QCOMPARE(tileTypeAt(game, base.y() - 2, base.x()), QString("empty"));
        QCOMPARE(tileTypeAt(game, base.y() - 1, base.x()), QString("crystal"));
        QCOMPARE(tileTypeAt(game, base.y(), base.x()), QString("crystal"));
        QCOMPARE(tileCells(game, "crystal").size(), initialCrystals.size());
        QCOMPARE(tileCells(game, "gold"), goldBeforePush);
        QCOMPARE(game.prizesLeft(), prizesBeforePush);
        QCOMPARE(game.score(), scoreBeforePush);
        QVERIFY(!game.gameOver());
        QCOMPARE(game.lives(), 3);
    }

    void startAtLevelLoadsAuthoredAndGeneratedMaps_data()
    {
        QTest::addColumn<int>("level");
        for (int level : {1, 6, 7, 50})
            QTest::newRow(qPrintable(QString::number(level))) << level;
    }

    void startAtLevelLoadsAuthoredAndGeneratedMaps()
    {
        QFETCH(int, level);
        QQmlEngine qmlEngine;
        QVariantList authoredLevels;
        QStringList expected;
        for (int number = 1; number <= 6; ++number) {
            const auto rows = loadQmlLevel(qmlEngine,
                    QStringLiteral("qrc:/levels/Level%1.qml").arg(number));
            QVERIFY(!rows.isEmpty());
            authoredLevels.append(QVariant::fromValue(asVariantRows(rows)));
            if (number == level)
                expected = rows;
        }
        if (level > 6) {
            const auto generated = LandscapeGenerator::generate(level - 6);
            QVERIFY(!generated.isEmpty());
            for (const auto &row : generated)
                expected.append(row);
        }

        GameEngine game;
        game.initializeLevels(authoredLevels);
        game.setPaused(true); // запуск должен работать и из меню
        QSignalSpy changed(&game, &GameEngine::levelChanged);
        QVERIFY(game.startAtLevel(level));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(game.currentLevel(), level);
        QCOMPARE(game.authoredLevelCount(), 6);
        QVERIFY(game.paused());
        QCOMPARE(game.rows(), expected.size());
        QCOMPARE(game.columns(), expected.first().size());
        QCOMPARE(game.prizesLeft(), countPrizes(expected));
        for (int row = 0; row < expected.size(); ++row) {
            for (int col = 0; col < expected[row].size(); ++col) {
                const QChar tile = expected[row][col];
                if (tile == 'P') {
                    QCOMPARE(game.playerX(), col);
                    QCOMPARE(game.playerY(), row);
                } else if (tile == 'C') {
                    QCOMPARE(tileTypeAt(game, row, col), QString("crystal"));
                } else if (tile == '$') {
                    QCOMPARE(tileTypeAt(game, row, col), QString("gold"));
                } else if (tile == 'X') {
                    QCOMPARE(tileTypeAt(game, row, col), QString("exit"));
                }
            }
        }
        if (level > 1) {
            game.setPaused(false);
            game.movePlayer(-1, 0);
            game.movePlayer(-1, 0);
            QCOMPARE(game.currentLevel(), level);
            QCOMPARE(game.prizesLeft(), countPrizes(expected));
            QCOMPARE(changed.count(), 1); // нижние карты не загружались
        }
        game.restartGame();
        QCOMPARE(game.currentLevel(), 1);
        QCOMPARE(game.authoredLevelCount(), 6);
        QCOMPARE(game.score(), 0);
        QCOMPARE(game.lives(), 3);
    }

    void startAtLevelRejectsInvalidRequestsWithoutChangingSession()
    {
        GameEngine uninitialized;
        QVERIFY(!uninitialized.startAtLevel(1));
        QCOMPARE(uninitialized.rows(), 0);
        auto rows = emptyLevel();
        setCell(rows, 3, 3, '$');
        GameEngine game;
        initialize(game, rows);
        game.movePlayer(1, 0);
        QVERIFY(game.score() > 0);
        const int score = game.score();
        QSignalSpy changed(&game, &GameEngine::levelChanged);
        for (int invalid : {-1, 0, 51, 1000}) {
            QVERIFY(!game.startAtLevel(invalid));
            QCOMPARE(game.currentLevel(), 1);
            QCOMPARE(game.score(), score);
            QCOMPARE(game.playerX(), 3);
            QCOMPARE(game.prizesLeft(), 0);
        }
        QCOMPARE(changed.count(), 0);
        game.setPaused(true);
        QVERIFY(game.startAtLevel(1));
        QCOMPARE(game.score(), 0);
        QCOMPARE(game.prizesLeft(), 1);
        QCOMPARE(game.playerX(), 2);
        QVERIFY(game.paused());
    }

    void testStartBlocksEarlierMapsAndPreservesForwardProgress()
    {
        auto rows = emptyLevel();
        setCell(rows, 3, 2, '.');
        setCell(rows, 5, 3, 'P');
        setCell(rows, 4, 4, '$');
        QVariantList levels;
        for (int number = 1; number <= 7; ++number)
            levels.append(QVariant::fromValue(asVariantRows(rows)));

        GameEngine game;
        game.initializeLevels(levels);
        QVERIFY(game.startAtLevel(6));
        QVERIFY(!game.canGoBack());

        game.movePlayer(-1, 0);
        game.movePlayer(-1, 0); // левый выход стартового уровня
        QCOMPARE(game.currentLevel(), 6);
        QCOMPARE(game.prizesLeft(), 1);
        QCOMPARE(game.score(), 0);

        for (int step = 0; step < 6; ++step)
            game.movePlayer(1, 0); // правый выход до сбора Приза
        QCOMPARE(game.currentLevel(), 6);
        QVERIFY(!game.levelComplete());

        game.movePlayer(0, -1);
        for (int step = 0; step < 3; ++step)
            game.movePlayer(-1, 0); // Приз в (4, 4)
        QCOMPARE(game.prizesLeft(), 0);
        QCOMPARE(game.score(), 100);
        QVERIFY(game.levelComplete());
        game.movePlayer(0, 1);
        for (int step = 0; step < 3; ++step)
            game.movePlayer(1, 0);
        QCOMPARE(game.currentLevel(), 7);
        QVERIFY(game.canGoBack());
        QCOMPARE(game.prizesLeft(), 1);

        game.movePlayer(-1, 0);
        game.movePlayer(-1, 0); // возврат к уже пройденному тестовому экрану
        QCOMPARE(game.currentLevel(), 6);
        QVERIFY(!game.canGoBack());
        QCOMPARE(game.playerX(), 7);
        QCOMPARE(game.prizesLeft(), 0);
        QCOMPARE(game.score(), 100);
        QCOMPARE(tileTypeAt(game, 4, 4), QString("empty"));

        for (int step = 0; step < 6; ++step)
            game.movePlayer(-1, 0);
        QCOMPARE(game.currentLevel(), 6); // граница сохраняется после возврата
        QCOMPARE(game.prizesLeft(), 0);
        for (int step = 0; step < 6; ++step)
            game.movePlayer(1, 0);
        QCOMPARE(game.currentLevel(), 7); // повторный выход вперёд доступен
        QCOMPARE(game.prizesLeft(), 1);
        QCOMPARE(game.score(), 100);
    }

    void testStartBoundarySurvivesInvalidRequestsAndResetsWithNewSession()
    {
        auto rows = emptyLevel();
        setCell(rows, 3, 2, '.');
        setCell(rows, 5, 3, 'P');
        QVariantList levels;
        for (int number = 1; number <= 7; ++number)
            levels.append(QVariant::fromValue(asVariantRows(rows)));

        GameEngine game;
        game.initializeLevels(levels);
        QVERIFY(game.startAtLevel(6));
        for (int invalid : {0, 51}) {
            QVERIFY(!game.startAtLevel(invalid));
            const int steps = game.playerX() - 1;
            for (int step = 0; step < steps; ++step)
                game.movePlayer(-1, 0);
            QCOMPARE(game.currentLevel(), 6);
            game.movePlayer(1, 0); // выйти с портала перед следующей проверкой
        }

        game.restartGame();
        QCOMPARE(game.currentLevel(), 1);
        for (int step = 0; step < 4; ++step)
            game.movePlayer(1, 0);
        QCOMPARE(game.currentLevel(), 2);
        game.movePlayer(-1, 0);
        game.movePlayer(-1, 0);
        QCOMPARE(game.currentLevel(), 1); // обычный возврат 2 → 1 восстановлен

        QVERIFY(game.startAtLevel(6));
        game.initializeLevels(levels);
        QCOMPARE(game.currentLevel(), 1);
        for (int step = 0; step < 4; ++step)
            game.movePlayer(1, 0);
        QCOMPARE(game.currentLevel(), 2);
        game.movePlayer(-1, 0);
        game.movePlayer(-1, 0);
        QCOMPARE(game.currentLevel(), 1); // повторная инициализация снимает границу
    }

    void pauseBlocksInputPhysicsAndPreservesPitTime()
    {
        auto rows = emptyLevel();
        setCell(rows, 4, 3, '#');
        setCell(rows, 1, 6, 'C');
        GameEngine game;
        initialize(game, rows);
        game.dig();
        QCOMPARE(tileTypeAt(game, 4, 3), QString("dug"));
        auto timers = game.findChildren<QTimer *>();
        QCOMPARE(timers.size(), 1);
        auto *timer = timers.first();
        timer->setTimerType(Qt::PreciseTimer);
        timer->start(400);
        QTest::qWait(100);
        const int remaining = timer->remainingTime();
        QVERIFY(remaining > 0);
        QSignalSpy pause(&game, &GameEngine::pausedChanged);
        game.setPaused(true);
        game.setPaused(true);
        QCOMPARE(pause.count(), 1);
        QVERIFY(!timer->isActive());
        const int x = game.playerX(), y = game.playerY(), score = game.score();
        const bool facing = game.playerFacingRight();
        QSignalSpy cells(game.tilesModel(), &QAbstractItemModel::dataChanged);
        game.movePlayer(-1, 0); game.dig(); game.updateCrystals(); game.updateGame();
        QTest::qWait(450); // Дольше всего исходного интервала таймера.
        QCOMPARE(game.playerX(), x); QCOMPARE(game.playerY(), y);
        QCOMPARE(game.playerFacingRight(), facing); QCOMPARE(game.score(), score);
        QCOMPARE(cells.count(), 0);
        QCOMPARE(tileTypeAt(game, 4, 3), QString("dug"));
        game.setPaused(false);
        QCOMPARE(pause.count(), 2);
        QVERIFY(timer->isActive());
        QVERIFY(qAbs(timer->remainingTime() - remaining) < 80);
        QTRY_COMPARE_WITH_TIMEOUT(tileTypeAt(game, 4, 3), QString("brick"), 700);
        game.movePlayer(-1, 0);
        QCOMPARE(game.playerX(), x - 1);
    }

    void repeatedPausePreservesSeveralIndependentTimers()
    {
        auto rows = emptyLevel();
        setCell(rows, 3, 2, '.');
        setCell(rows, 3, 4, 'P');
        setCell(rows, 4, 5, '#');
        setCell(rows, 4, 2, '#');
        setCell(rows, 4, 3, '#');
        GameEngine game;
        initialize(game, rows);
        game.dig();
        game.movePlayer(-1, 0);
        game.dig();
        auto timers = game.findChildren<QTimer *>();
        QCOMPARE(timers.size(), 2);
        timers[0]->setTimerType(Qt::PreciseTimer); timers[0]->start(700);
        timers[1]->setTimerType(Qt::PreciseTimer); timers[1]->start(1100);
        for (int cycle = 0; cycle < 2; ++cycle) {
            QHash<QTimer *, int> remaining;
            for (auto *timer : timers) remaining.insert(timer, timer->remainingTime());
            game.setPaused(true);
            for (auto *timer : timers) QVERIFY(!timer->isActive());
            QTest::qWait(60);
            game.setPaused(false);
            for (auto *timer : timers) {
                QVERIFY(timer->isActive());
                QVERIFY(qAbs(timer->remainingTime() - remaining.value(timer)) < 40);
            }
        }
    }

    void restartWhilePausedDiscardsOldPitTimers()
    {
        auto rows = emptyLevel();
        setCell(rows, 4, 3, '#');
        GameEngine game;
        initialize(game, rows);
        game.dig();
        game.setPaused(true);
        game.restartGame();
        QVERIFY(game.paused());
        QCOMPARE(tileTypeAt(game, 4, 3), QString("brick"));
        game.setPaused(false);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(game.findChildren<QTimer *>().isEmpty());
        game.dig();
        auto timers = game.findChildren<QTimer *>();
        QCOMPARE(timers.size(), 1);
        QVERIFY(timers.first()->isActive());
        QVERIFY(timers.first()->remainingTime() > 14000);
    }

    void collectingPrizeUpdatesScoreAndUnlocksExit()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 3, 3, QLatin1Char('$'));

        GameEngine game;
        initialize(game, rows);
        QCOMPARE(game.prizesLeft(), 1);
        QVERIFY(!game.levelComplete());

        game.movePlayer(1, 0);

        QCOMPARE(game.score(), 100);
        QCOMPARE(game.prizesLeft(), 0);
        QVERIFY(game.levelComplete());
    }

    void collectingPrizeOnActualFirstLevelUpdatesScoreAndHud()
    {
        QQmlEngine qmlEngine;
        const QStringList rows = loadQmlLevel(qmlEngine, QStringLiteral("qrc:/levels/Level1.qml"));
        QVERIFY(!rows.isEmpty());

        GameEngine game;
        initialize(game, rows);
        QSignalSpy scoreChanged(&game, &GameEngine::scoreChanged);
        QCOMPARE(game.score(), 0);

        // На первом уровне старт находится в (строка 11, столбец 3),
        // ближайший Приз — в той же строке, в столбце 6.
        game.movePlayer(1, 0);
        game.movePlayer(1, 0);
        game.movePlayer(1, 0);

        QCOMPARE(game.score(), 100);
        QCOMPARE(game.prizesLeft(), countPrizes(rows) - 1);
        QVERIFY(scoreChanged.count() >= 1);
    }

    void collectingKeyAndOpeningSafeShowsNotices()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 3, 3, QLatin1Char('K'));
        setCell(rows, 3, 4, QLatin1Char('B'));

        GameEngine game;
        initialize(game, rows);
        QSignalSpy keyChanged(&game, &GameEngine::hasKeyChanged);

        game.movePlayer(1, 0);

        QVERIFY(game.hasKey());
        QVERIFY(game.keyCollectedNotice());
        QCOMPARE(keyChanged.count(), 1);
        game.dismissKeyCollectedNotice();
        QVERIFY(!game.keyCollectedNotice());

        game.movePlayer(1, 0);
        QVERIFY(!game.hasKey());
        QCOMPARE(game.score(), 100);
        QVERIFY(game.safeOpenedNotice());
        game.dismissSafeOpenedNotice();
        QVERIFY(!game.safeOpenedNotice());
    }

    void approachingLockedSafeShowsNotice()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 3, 4, QLatin1Char('B'));

        GameEngine game;
        initialize(game, rows);
        QVERIFY(!game.keyRequiredNotice());

        game.movePlayer(1, 0);

        QVERIFY(game.keyRequiredNotice());
        QCOMPARE(game.playerX(), 3);
        game.dismissKeyRequiredNotice();
        QVERIFY(!game.keyRequiredNotice());
        game.movePlayer(1, 0);
        QVERIFY(!game.keyRequiredNotice());
        QCOMPARE(game.playerX(), 3);
    }

    void safeNoticeCanAppearAgainOnNextLevelVisit()
    {
        QStringList firstLevel = emptyLevel();
        setCell(firstLevel, 3, 4, QLatin1Char('B'));
        const QStringList secondLevel = emptyLevel();
        QVariantList authoredLevels;
        authoredLevels.append(QVariant::fromValue(asVariantRows(firstLevel)));
        authoredLevels.append(QVariant::fromValue(asVariantRows(secondLevel)));

        GameEngine game;
        game.initializeLevels(authoredLevels);
        game.movePlayer(1, 0);
        QVERIFY(game.keyRequiredNotice());
        game.dismissKeyRequiredNotice();

        game.movePlayer(1, 0); // повторная попытка войти в Сейф в этом визите
        QVERIFY(!game.keyRequiredNotice());
        game.movePlayer(0, 1);
        game.movePlayer(0, 1);
        for (int step = 0; step < 4; ++step)
            game.movePlayer(1, 0); // переход на уровень 2
        QCOMPARE(game.currentLevel(), 2);

        game.movePlayer(0, 1);
        game.movePlayer(0, 1);
        game.movePlayer(-1, 0); // возврат на уровень 1 через левый портал
        QCOMPARE(game.currentLevel(), 1);
        game.movePlayer(-1, 0);
        game.movePlayer(-1, 0);
        game.movePlayer(0, -1);
        game.movePlayer(0, -1); // снова вплотную к Сейфу

        QVERIFY(game.keyRequiredNotice());
    }

    void remainingPrizeBlocksExit()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 3, 3, QLatin1Char('$'));
        setCell(rows, 3, 2, QLatin1Char('.'));
        setCell(rows, 5, 6, QLatin1Char('P'));

        GameEngine game;
        initialize(game, rows);
        QCOMPARE(game.prizesLeft(), 1);

        game.movePlayer(1, 0);

        QCOMPARE(game.currentLevel(), 1);
        QCOMPARE(game.prizesLeft(), 1);
    }

    void fallingCrystalKillsStationaryPlayer()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 2, 3, QLatin1Char('C'));
        setCell(rows, 3, 2, QLatin1Char('.'));
        setCell(rows, 4, 2, QLatin1Char('P'));

        GameEngine game;
        initialize(game, rows);
        game.updateCrystals(); // Кристалл падает на одну клетку.
        game.movePlayer(1, 0); // Персонаж встаёт под уже падающий Кристалл.
        game.updateCrystals(); // Даём Персонажу один шаг после перемещения.
        QVERIFY(!game.gameOver());

        game.updateCrystals();

        QVERIFY(game.gameOver());
        QCOMPARE(game.lives(), 2);
    }

    void stationaryCrystalDirectlyAbovePlayerIsNotLethal()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 3, 3, QLatin1Char('C'));
        setCell(rows, 3, 2, QLatin1Char('.'));
        setCell(rows, 4, 3, QLatin1Char('P'));

        GameEngine game;
        initialize(game, rows);
        game.updateCrystals();

        QVERIFY(!game.gameOver());
        QCOMPARE(game.lives(), 3);
    }

    void playerCanEnterAndLeaveDugBrickOnLevelOne()
    {
        QQmlEngine qmlEngine;
        const QStringList rows = loadQmlLevel(qmlEngine, QStringLiteral("qrc:/levels/Level1.qml"));
        QVERIFY(!rows.isEmpty());

        GameEngine game;
        initialize(game, rows);
        for (int step = 0; step < 10; ++step)
            game.movePlayer(0, -1); // из старта на row 1, column 3
        for (int step = 0; step < 4; ++step)
            game.movePlayer(1, 0); // подходим к кирпичу row 2, column 8

        QCOMPARE(game.playerY(), 1);
        QCOMPARE(game.playerX(), 7);
        game.dig();
        QCOMPARE(tileTypeAt(game, 2, 8), QStringLiteral("dug"));

        game.movePlayer(0, 1);  // входим в открытую клетку снизу
        game.movePlayer(1, 0); // проходим через неё по горизонтали
        QCOMPARE(game.playerY(), 2);
        QCOMPARE(game.playerX(), 8);
        game.movePlayer(-1, 0);
        game.movePlayer(1, 0); // проверяем повторный вход
        QCOMPARE(game.playerX(), 8);
        game.movePlayer(-1, 0); // покидаем яму до восстановления

        const QList<QTimer *> timers = game.findChildren<QTimer *>();
        QCOMPARE(timers.size(), 1);
        timers.first()->setInterval(1);
        QTRY_COMPARE(tileTypeAt(game, 2, 8), QStringLiteral("brick"));
        QVERIFY(!game.gameOver());
    }

    void restoredBrickKillsPlayerWhoRemainsInDugCellOnLevelOne()
    {
        QQmlEngine qmlEngine;
        const QStringList rows = loadQmlLevel(qmlEngine, QStringLiteral("qrc:/levels/Level1.qml"));
        QVERIFY(!rows.isEmpty());

        GameEngine game;
        initialize(game, rows);
        for (int step = 0; step < 10; ++step)
            game.movePlayer(0, -1);
        for (int step = 0; step < 4; ++step)
            game.movePlayer(1, 0);
        game.dig();
        game.movePlayer(0, 1);
        game.movePlayer(1, 0);
        QCOMPARE(game.playerX(), 8);
        QCOMPARE(game.playerY(), 2);

        const QList<QTimer *> timers = game.findChildren<QTimer *>();
        QCOMPARE(timers.size(), 1);
        timers.first()->setInterval(1);
        QTRY_VERIFY(game.gameOver());
        QCOMPARE(game.lives(), 2);
        QCOMPARE(tileTypeAt(game, 2, 8), QStringLiteral("brick"));
    }

    void crystalFallingIntoDugBrickBecomesBonusPrize()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 3, 3, QLatin1Char('C'));
        setCell(rows, 4, 3, QLatin1Char('#'));
        setCell(rows, 5, 3, QLatin1Char('#'));

        GameEngine game;
        initialize(game, rows);
        QCOMPARE(game.prizesLeft(), 0);

        game.dig();
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("crystal"));
        game.updateCrystals();
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("crystal"));

        const QList<QTimer *> dugTileTimers = game.findChildren<QTimer *>();
        QCOMPARE(dugTileTimers.size(), 1);
        dugTileTimers.first()->setInterval(1);
        QTRY_COMPARE(tileTypeAt(game, 4, 3), QStringLiteral("bonusPrize"));
        QCOMPARE(game.prizesLeft(), 0);
        QVERIFY(!dugTileTimers.first()->isActive());

        game.movePlayer(0, 1);
        game.movePlayer(1, 0);
        QCOMPARE(game.score(), 100);
        QCOMPARE(game.prizesLeft(), 0);
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("dug"));
        const QList<QTimer *> restartedTimers = game.findChildren<QTimer *>();
        QTimer *restorationTimer = nullptr;
        for (QTimer *timer : restartedTimers) {
            if (timer->isActive()) {
                restorationTimer = timer;
                break;
            }
        }
        QVERIFY(restorationTimer != nullptr);
        QCOMPARE(restorationTimer->interval(), 15000);
        game.movePlayer(-1, 0);
        restorationTimer->setInterval(1);
        QTRY_COMPARE(tileTypeAt(game, 4, 3), QStringLiteral("brick"));
    }

    void bonusPrizeRestoresBrickAndAllowsRepeatedDigging()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 2, 3, QLatin1Char('C'));
        setCell(rows, 3, 3, QLatin1Char('C'));
        setCell(rows, 4, 3, QLatin1Char('#'));
        setCell(rows, 5, 3, QLatin1Char('#'));

        GameEngine game;
        initialize(game, rows);

        auto activeDugTimer = [&game]() -> QTimer * {
            for (QTimer *timer : game.findChildren<QTimer *>()) {
                if (timer->isActive())
                    return timer;
            }
            return nullptr;
        };
        auto waitForBonusPrize = [&game, &activeDugTimer]() {
            QTimer *timer = activeDugTimer();
            QVERIFY(timer != nullptr);
            timer->setInterval(1);
            QTRY_COMPARE(tileTypeAt(game, 4, 3), QStringLiteral("bonusPrize"));
        };
        auto waitForBrick = [&game, &activeDugTimer]() {
            QTimer *timer = activeDugTimer();
            QVERIFY(timer != nullptr);
            timer->setInterval(1);
            QTRY_COMPARE(tileTypeAt(game, 4, 3), QStringLiteral("brick"));
        };

        game.dig();
        waitForBonusPrize();
        game.movePlayer(0, 1);
        game.movePlayer(1, 0);
        QCOMPARE(game.score(), 100);
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("dug"));
        game.movePlayer(-1, 0);
        waitForBrick(); // после сбора запускается полный таймер восстановления
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("brick"));

        game.movePlayer(0, -1);
        game.movePlayer(-1, 0);
        game.movePlayer(1, 0); // разворачиваемся вправо, не толкая Кристалл
        game.dig();
        game.updateCrystals(); // второй Кристалл падает в ту же яму
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("crystal"));
        waitForBonusPrize();

        game.movePlayer(0, 1);
        game.movePlayer(1, 0);
        QCOMPARE(game.score(), 200);
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("dug"));
        game.movePlayer(-1, 0);
        waitForBrick();
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("brick"));
    }

    void levelOneCanRepeatCrystalPrizeBrickCycle()
    {
        QQmlEngine qmlEngine;
        QStringList rows = loadQmlLevel(qmlEngine, QStringLiteral("qrc:/levels/Level1.qml"));
        QVERIFY(!rows.isEmpty());
        // Создаём два Кристалла над доступной ямой в нижней части реальной карты.
        setCell(rows, 9, 4, QLatin1Char('C'));
        setCell(rows, 10, 4, QLatin1Char('C'));
        setCell(rows, 11, 4, QLatin1Char('#'));

        GameEngine game;
        initialize(game, rows);
        const int requiredPrizes = game.prizesLeft();
        auto activeDugTimer = [&game]() -> QTimer * {
            for (QTimer *timer : game.findChildren<QTimer *>()) {
                if (timer->isActive())
                    return timer;
            }
            return nullptr;
        };
        auto waitForBonus = [&game, &activeDugTimer]() {
            QTimer *timer = activeDugTimer();
            QVERIFY(timer != nullptr);
            timer->setInterval(1);
            QTRY_COMPARE(tileTypeAt(game, 11, 4), QStringLiteral("bonusPrize"));
        };
        auto waitForBrick = [&game, &activeDugTimer]() {
            QTimer *timer = activeDugTimer();
            QVERIFY(timer != nullptr);
            timer->setInterval(1);
            QTRY_COMPARE(tileTypeAt(game, 11, 4), QStringLiteral("brick"));
        };

        game.movePlayer(0, -1); // клетка подхода над стартом
        game.dig();
        QCOMPARE(tileTypeAt(game, 11, 4), QStringLiteral("crystal"));
        waitForBonus();

        game.movePlayer(0, 1);
        game.movePlayer(1, 0);
        QCOMPARE(game.score(), 100);
        QCOMPARE(tileTypeAt(game, 11, 4), QStringLiteral("dug"));
        game.movePlayer(-1, 0);
        waitForBrick();
        QCOMPARE(tileTypeAt(game, 11, 4), QStringLiteral("brick"));

        // Возвращаемся к той же клетке и копаем её повторно.
        game.movePlayer(0, -1);
        game.movePlayer(-1, 0);
        game.movePlayer(1, 0);
        game.dig();
        game.updateCrystals();
        QCOMPARE(tileTypeAt(game, 11, 4), QStringLiteral("crystal"));
        waitForBonus();
        QCOMPARE(game.score(), 100);
        QCOMPARE(game.prizesLeft(), requiredPrizes);
    }

    void levelFiveCrystalFallsThroughDugBrickIfBelowIsEmpty()
    {
        QQmlEngine qmlEngine;
        QStringList rows = loadQmlLevel(qmlEngine, QStringLiteral("qrc:/levels/Level5.qml"));
        QVERIFY(!rows.isEmpty());
        // Подготавливаем воспроизводимый сценарий на карте уровня 5:
        // Кристалл находится над кирпичом в row 10, column 15.
        setCell(rows, 9, 15, QLatin1Char('C'));

        GameEngine game;
        initialize(game, rows);
        const int requiredPrizesBefore = game.prizesLeft();

        game.movePlayer(0, -1); // (row 10, column 3)
        game.movePlayer(0, -1); // (row 9, column 3)
        for (int step = 0; step < 11; ++step)
            game.movePlayer(1, 0); // идём к клетке подхода (row 9, column 14)
        QCOMPARE(game.playerX(), 14);
        QCOMPARE(game.playerY(), 9);
        QCOMPARE(game.score(), 100); // по пути собран обычный Приз в column 4

        game.dig();
        QCOMPARE(tileTypeAt(game, 10, 15), QStringLiteral("crystal"));
        game.updateCrystals();
        QCOMPARE(tileTypeAt(game, 10, 15), QStringLiteral("empty"));
        QCOMPARE(tileTypeAt(game, 11, 15), QStringLiteral("crystal"));

        const QList<QTimer *> dugTileTimers = game.findChildren<QTimer *>();
        QCOMPARE(dugTileTimers.size(), 1);
        dugTileTimers.first()->setInterval(1);
        QTRY_COMPARE(tileTypeAt(game, 10, 15), QStringLiteral("brick"));
        QCOMPARE(tileTypeAt(game, 11, 15), QStringLiteral("crystal"));
        QCOMPARE(game.score(), 100);
        QCOMPARE(game.prizesLeft(), requiredPrizesBefore - 1);
    }

    void authoredLevelsThreeAndFourHaveRequiredPrizes()
    {
        QQmlEngine qmlEngine;
        const QList<QPair<QString, QString>> levelUrls = {
            {QStringLiteral("qrc:/levels/Level3.qml"), QStringLiteral("уровень 3")},
            {QStringLiteral("qrc:/levels/Level4.qml"), QStringLiteral("уровень 4")}
        };

        for (const auto &entry : levelUrls) {
            const QStringList rows = loadQmlLevel(qmlEngine, entry.first);
            QVERIFY2(!rows.isEmpty(), qPrintable(QStringLiteral("Не удалось загрузить %1")
                                                  .arg(entry.second)));

            GameEngine game;
            initialize(game, rows);

            QVERIFY2(countPrizes(rows) > 0, qPrintable(QStringLiteral("На %1 нет Призов")
                                                        .arg(entry.second)));
            QCOMPARE(game.prizesLeft(), countPrizes(rows));
        }
    }
};

QTEST_GUILESS_MAIN(GameEngineTest)
#include "test_gameengine.moc"
