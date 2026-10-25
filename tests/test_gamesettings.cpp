#include "../src/gamesettings.h"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class GameSettingsTest : public QObject
{
    Q_OBJECT
private slots:
    void persistsChangesAndNotifiesOnce()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath("settings.ini");
        {
            GameSettings settings(file);
            QCOMPARE(settings.tileTheme(), QString("classic"));
            QCOMPARE(settings.playerVariant(), QString("classic"));
            QVERIFY(settings.animationsEnabled());
            QCOMPARE(settings.animationInterval(), 140);
            QSignalSpy theme(&settings, &GameSettings::tileThemeChanged);
            QSignalSpy player(&settings, &GameSettings::playerVariantChanged);
            QSignalSpy animations(&settings, &GameSettings::animationsEnabledChanged);
            QSignalSpy speed(&settings, &GameSettings::animationIntervalChanged);
            settings.setTileTheme("industrial"); settings.setTileTheme("industrial");
            settings.setPlayerVariant("robot"); settings.setPlayerVariant("robot");
            settings.setAnimationsEnabled(false); settings.setAnimationsEnabled(false);
            settings.setAnimationInterval(220); settings.setAnimationInterval(220);
            QCOMPARE(theme.count(), 1); QCOMPARE(player.count(), 1);
            QCOMPARE(animations.count(), 1); QCOMPARE(speed.count(), 1);
        }
        GameSettings restored(file);
        QCOMPARE(restored.tileTheme(), QString("industrial"));
        QCOMPARE(restored.playerVariant(), QString("robot"));
        QVERIFY(!restored.animationsEnabled());
        QCOMPARE(restored.animationInterval(), 220);
        restored.resetDefaults();
        GameSettings defaults(file);
        QCOMPARE(defaults.tileTheme(), QString("classic"));
        QCOMPARE(defaults.playerVariant(), QString("classic"));
        QVERIFY(defaults.animationsEnabled());
        QCOMPARE(defaults.animationInterval(), 140);
    }

    void rejectsInvalidValuesAndRepairsDefaults()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath("settings.ini");
        {
            QSettings storage(file, QSettings::IniFormat);
            storage.setValue("appearance/tileTheme", "missing");
            storage.setValue("appearance/playerVariant", "missing");
            storage.setValue("appearance/animationInterval", "bad");
            storage.setValue("appearance/animationsEnabled", "bad");
            storage.setValue("highScore", 123);
        }
        GameSettings settings(file);
        QCOMPARE(settings.tileTheme(), QString("classic"));
        QCOMPARE(settings.playerVariant(), QString("classic"));
        QCOMPARE(settings.animationInterval(), 140);
        QVERIFY(settings.animationsEnabled());
        QSignalSpy theme(&settings, &GameSettings::tileThemeChanged);
        QSignalSpy player(&settings, &GameSettings::playerVariantChanged);
        QSignalSpy speed(&settings, &GameSettings::animationIntervalChanged);
        settings.setTileTheme("missing"); settings.setPlayerVariant("missing");
        settings.setAnimationInterval(0); settings.setAnimationInterval(501);
        QCOMPARE(theme.count(), 0); QCOMPARE(player.count(), 0); QCOMPARE(speed.count(), 0);
        settings.resetDefaults();
        QSettings stored(file, QSettings::IniFormat);
        QCOMPARE(stored.value("appearance/tileTheme").toString(), QString("classic"));
        QCOMPARE(stored.value("appearance/playerVariant").toString(), QString("classic"));
        QCOMPARE(stored.value("appearance/animationInterval").toInt(), 140);
        QVERIFY(stored.value("appearance/animationsEnabled").toBool());
        QCOMPARE(stored.value("highScore").toInt(), 123);
    }
};
QTEST_GUILESS_MAIN(GameSettingsTest)
#include "test_gamesettings.moc"
