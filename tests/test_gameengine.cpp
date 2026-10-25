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
