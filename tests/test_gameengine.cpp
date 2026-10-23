#include "../src/gameengine.h"

#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QStandardPaths>
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

    void collectingKeyEmitsPropertyChange()
    {
        QStringList rows = emptyLevel();
        setCell(rows, 3, 3, QLatin1Char('K'));

        GameEngine game;
        initialize(game, rows);
        QSignalSpy keyChanged(&game, &GameEngine::hasKeyChanged);

        game.movePlayer(1, 0);

        QVERIFY(game.hasKey());
        QCOMPARE(keyChanged.count(), 1);
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
        QVERIFY(game.keyRequiredNotice());
        QCOMPARE(game.playerX(), 3);
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
