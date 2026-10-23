#include "../src/gameengine.h"

#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTimer>
#include <QtTest>

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

        game.movePlayer(0, 1);
        game.movePlayer(1, 0);
        QCOMPARE(game.score(), 100);
        QCOMPARE(game.prizesLeft(), 0);
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

        game.dig();
        waitForBonusPrize();
        game.movePlayer(0, 1);
        game.movePlayer(1, 0);
        QCOMPARE(game.score(), 100);
        game.movePlayer(-1, 0); // покидаем клетку Приза, Кирпич восстанавливается
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
        game.movePlayer(-1, 0);
        QCOMPARE(tileTypeAt(game, 4, 3), QStringLiteral("brick"));
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
