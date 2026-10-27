#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QtTest>
#include "../src/gameengine.h"
#include "../src/gamesettings.h"
#include <QTemporaryDir>
#include <QSettings>

class TileLibraryTest : public QObject
{
    Q_OBJECT
    QTemporaryDir m_configDir;
private slots:
    void initTestCase()
    {
        QVERIFY(m_configDir.isValid());
        qputenv("XDG_CONFIG_HOME", m_configDir.path().toUtf8());
        QCoreApplication::setOrganizationName("BoulderDashTests");
        QCoreApplication::setApplicationName("InterfaceTests");
        qmlRegisterType<GameSettings>("BoulderDash", 1, 0, "GameSettings");
        qRegisterMetaType<QList<QQmlError>>("QList<QQmlError>");
        qmlRegisterType<GameEngine>("BoulderDash", 1, 0, "GameEngine");
    }

    void init() { QSettings().clear(); }

    void rendersTiles_data()
    {
        QTest::addColumn<QString>("componentName");
        QTest::addColumn<QString>("theme");
        QTest::addColumn<int>("size");
        for (const auto &component : {"BrickTile", "CrystalTile", "PrizeTile"})
            for (const auto &theme : {"classic", "kenney", "industrial"})
                for (int size : {18, 32, 64}) {
                    const QByteArray name = QByteArray(component) + '-' + theme + '-' + QByteArray::number(size);
                    QTest::newRow(name.constData()) << QString(component) << QString(theme) << size;
                }
    }

    void rendersTiles()
    {
        QFETCH(QString, componentName);
        QFETCH(QString, theme);
        QFETCH(int, size);
        QQmlEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        QQmlComponent component(&engine, QUrl("qrc:/components/tiles/" + componentName + ".qml"));
        QScopedPointer<QObject> tile(component.create());
        QVERIFY2(tile, qPrintable(component.errorString()));
        QVERIFY(tile->setProperty("animated", false));
        QVERIFY(tile->setProperty("cellSize", size));
        QVERIFY(tile->setProperty("theme", theme));
        QCOMPARE(tile->property("width").toInt(), size);
        QCOMPARE(tile->property("height").toInt(), size);
        QCOMPARE(tile->property("usesSprite").toBool(), theme != "classic");
        if (theme != "classic")
            QTRY_COMPARE(tile->property("imageStatus").toInt(), 1); // Image.Ready
        const auto semanticType = tile->property("tileType");
        // Switching the same instance must update the source without changing its meaning.
        QVERIFY(tile->setProperty("theme", "industrial"));
        QTRY_COMPARE(tile->property("imageStatus").toInt(), 1);
        QVERIFY(tile->setProperty("theme", "classic"));
        QVERIFY(!tile->property("usesSprite").toBool());
        QCOMPARE(tile->property("tileType"), semanticType);
        QCOMPARE(warnings.count(), 0);
    }

    void playerVariants_data()
    {
        QTest::addColumn<QString>("variant");
        QTest::addColumn<QString>("action");
        QTest::addColumn<bool>("right");
        QTest::addColumn<int>("size");
        for (const auto &variant : {"classic", "astronaut", "pink", "robot"})
            for (const auto &action : {"idle", "run", "climb", "dig", "fall"})
                for (bool right : {true, false})
                    for (int size : {32, 64}) {
                        const QByteArray name = QByteArray(variant) + '-' + action
                            + (right ? "-right-" : "-left-") + QByteArray::number(size);
                        QTest::newRow(name.constData()) << QString(variant) << QString(action) << right << size;
                    }
    }

    void playerVariants()
    {
        QFETCH(QString, variant);
        QFETCH(QString, action);
        QFETCH(bool, right);
        QFETCH(int, size);
        QQmlEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        QQmlComponent component(&engine, QUrl("qrc:/components/Player.qml"));
        QScopedPointer<QObject> player(component.create());
        QVERIFY2(player, qPrintable(component.errorString()));
        player->setProperty("animated", false);
        player->setProperty("variant", variant);
        player->setProperty("state", action);
        player->setProperty("size", size);
        player->setProperty("facingRight", right);
        QCOMPARE(player->property("width").toInt(), size);
        QCOMPARE(player->property("height").toInt(), size);
        QCOMPARE(player->property("state").toString(), action);
        QCOMPARE(player->property("usesSprite").toBool(), variant != "classic");
        if (variant != "classic")
            QTRY_COMPARE(player->property("imageStatus").toInt(), 1);
        auto *sprite = player->findChild<QObject *>("playerSprite");
        QVERIFY(sprite);
        QCOMPARE(sprite->property("mirror").toBool(), !right);
        player->setProperty("variant", "unknown");
        QVERIFY(!player->property("usesSprite").toBool());
        QCOMPARE(player->property("state").toString(), action);
        QCOMPARE(player->property("facingRight").toBool(), right);
        QCOMPARE(warnings.count(), 0);
    }

    void playerAnimationCanBeStopped()
    {
        QQmlEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        QQmlComponent component(&engine, QUrl("qrc:/components/Player.qml"));
        QScopedPointer<QObject> player(component.create());
        QVERIFY2(player, qPrintable(component.errorString()));
        player->setProperty("variant", "robot");
        player->setProperty("state", "run");
        const QUrl firstFrame = player->property("spriteSource").toUrl();
        QTRY_VERIFY(player->property("spriteSource").toUrl() != firstFrame);
        QTRY_COMPARE(player->property("imageStatus").toInt(), 1);
        player->setProperty("animated", false);
        QCOMPARE(player->property("spriteSource").toUrl(), firstFrame);
        QTest::qWait(200);
        QCOMPARE(player->property("spriteSource").toUrl(), firstFrame);
        QCOMPARE(warnings.count(), 0);
    }

    void galleryLoadsAndResizes()
    {
        QQmlApplicationEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        engine.load(QUrl("qrc:/examples/TileGallery.qml"));
        QCOMPARE(engine.rootObjects().size(), 1);
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(window->findChildren<GameEngine *>().isEmpty());
        QTest::qWait(100);
        for (int size : {18, 32, 48, 64}) {
            window->setProperty("previewSize", size);
            window->setProperty("highlightTiles", true);
            window->setProperty("animateTiles", true);
            QTest::qWait(20);
        }
        window->resize(400, 540);
        QTest::qWait(50);
        window->resize(960, 680);
        window->setProperty("previewSize", 32);
        window->setProperty("highlightTiles", false);
        window->setProperty("animateTiles", false);
        QTest::qWait(100);
        const auto screenshotPath = qgetenv("QML_GALLERY_SCREENSHOT");
        if (!screenshotPath.isEmpty()) {
            window->resize(960, 1040);
            QTest::qWait(100);
            auto grab = window->contentItem()->grabToImage();
            QVERIFY(grab);
            QSignalSpy ready(grab.data(), &QQuickItemGrabResult::ready);
            if (grab->image().isNull())
                QVERIFY(ready.wait(5000));
            QVERIFY(!grab->image().isNull());
            QVERIFY(grab->image().save(QString::fromLocal8Bit(screenshotPath)));
        }
        QCOMPARE(warnings.count(), 0);
    }

    void menuSettingsAndPause()
    {
        QQmlApplicationEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        engine.load(QUrl("qrc:/main.qml"));
        QCOMPARE(engine.rootObjects().size(), 1);
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        auto *game = window->findChild<GameEngine *>();
        auto *settings = window->findChild<GameSettings *>();
        auto *physics = window->findChild<QObject *>("physicsTimer");
        QVERIFY(game); QVERIFY(settings); QVERIFY(physics);
        QTest::qWait(50);
        auto click = [window](const char *name) {
            auto *button = window->findChild<QQuickItem *>(name);
            if (!button || !button->isVisible()) return false;
            auto center = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center, 0);
            QCoreApplication::processEvents();
            return true;
        };
        QCOMPARE(window->property("overlay").toString(), QString("menu"));
        QVERIFY(game->paused());
        QVERIFY(!physics->property("running").toBool());
        QVERIFY(click("openSettingsButton"));
        QCOMPARE(window->property("overlay").toString(), QString("settings"));
        QVERIFY(click("settingsThemeButton"));
        QCOMPARE(settings->tileTheme(), QString("kenney"));
        QVERIFY(click("settingsPlayerButton"));
        QCOMPARE(settings->playerVariant(), QString("astronaut"));
        QVERIFY(click("settingsAnimationsButton"));
        QVERIFY(!settings->animationsEnabled());
        QVERIFY(click("settingsSpeedButton"));
        QCOMPARE(settings->animationInterval(), 220);
        const auto snapshot = qgetenv("QML_SETTINGS_SCREENSHOT");
        if (!snapshot.isEmpty()) {
            QTest::qWait(100);
            auto grab = window->contentItem()->grabToImage();
            QVERIFY(grab);
            QSignalSpy ready(grab.data(), &QQuickItemGrabResult::ready);
            if (grab->image().isNull()) QVERIFY(ready.wait(5000));
            QVERIFY(grab->image().save(QString::fromLocal8Bit(snapshot)));
        }
        QVERIFY(click("closeSettingsButton"));
        QVERIFY(game->paused());
        QVERIFY(click("newGameButton"));
        QVERIFY(!game->paused());
        QVERIFY(physics->property("running").toBool());
        QCOMPARE(window->property("overlay").toString(), QString(""));
        auto *player = window->findChild<QObject *>("gamePlayer");
        QVERIFY(player);
        QCOMPARE(player->property("variant").toString(), QString("astronaut"));
        QCOMPARE(player->property("animationInterval").toInt(), 220);
        QVERIFY(!player->property("animated").toBool());
        QVERIFY(click("toolbarSettingsButton"));
        QVERIFY(game->paused());
        QVERIFY(click("closeSettingsButton"));
        QVERIFY(!game->paused());
        QVERIFY(click("pauseButton"));
        QVERIFY(game->paused());
        const int x = game->playerX(), y = game->playerY();
        QTest::keyClick(window, Qt::Key_Right);
        QCOMPARE(game->playerX(), x); QCOMPARE(game->playerY(), y);
        QVERIFY(click("resumeButton"));
        QVERIFY(!game->paused());
        window->requestActivate();
        QTest::qWait(50);
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(game->paused());
        QTest::keyClick(window, Qt::Key_P);
        QTRY_VERIFY(!game->paused());
        // Новый объект настроек читает значения, сохранённые UI.
        GameSettings restored;
        QCOMPARE(restored.tileTheme(), QString("kenney"));
        QCOMPARE(restored.playerVariant(), QString("astronaut"));
        QCOMPARE(restored.animationInterval(), 220);
        QCOMPARE(warnings.count(), 0);
    }

    void testStartKeepsLeftExitUnavailableAfterCollectingPrizes()
    {
        QQmlApplicationEngine engine;
        engine.load(QUrl("qrc:/main.qml"));
        QCOMPARE(engine.rootObjects().size(), 1);
        QObject *root = engine.rootObjects().first();
        auto *game = root->findChild<GameEngine *>();
        QVERIFY(game);
        QVariantList rows;
        for (const auto &row : {"#########", "#.......#", "#.......#", "#X.P...X#", "#########"})
            rows.append(QString::fromLatin1(row));
        QVariantList levels;
        for (int index = 0; index < 6; ++index)
            levels.append(QVariant::fromValue(rows));
        game->initializeLevels(levels);
        QVERIFY(game->startAtLevel(6));
        QCOMPARE(game->prizesLeft(), 0);
        QCoreApplication::processEvents();
        auto *window = qobject_cast<QQuickWindow *>(root);
        QVERIFY(window);
        QList<QQuickItem *> pending{window->contentItem()};
        int exits = 0;
        while (!pending.isEmpty()) {
            auto *item = pending.takeLast();
            pending.append(item->childItems());
            if (item->property("type").toString() != "exit")
                continue;
            ++exits;
            const bool left = qRound(item->x()) == 32;
            QCOMPARE(item->property("exitAvailable").toBool(), !left);
        }
        QCOMPARE(exits, 2);
        QVERIFY(QMetaObject::invokeMethod(root, "newGame"));
        QCOMPARE(game->currentLevel(), 1);
        QVERIFY(!game->canGoBack());
    }

    void gameChangesThemeWithoutChangingState()
    {
        QQmlApplicationEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        engine.load(QUrl("qrc:/main.qml"));
        QCOMPARE(engine.rootObjects().size(), 1);
        QObject *root = engine.rootObjects().first();
        auto *game = root->findChild<GameEngine *>();
        QVERIFY(game);
        QVERIFY(QMetaObject::invokeMethod(root, "newGame"));
        const int score = game->score();
        const int prizes = game->prizesLeft();
        const int x = game->playerX();
        const int y = game->playerY();
        for (const auto &theme : {"kenney", "industrial", "classic"}) {
            root->setProperty("tileTheme", theme);
            QCoreApplication::processEvents();
            QCOMPARE(game->score(), score);
            QCOMPARE(game->prizesLeft(), prizes);
            QCOMPARE(game->playerX(), x);
            QCOMPARE(game->playerY(), y);
        }
        auto *player = root->findChild<QObject *>("gamePlayer");
        QVERIFY(player);
        auto *button = root->findChild<QQuickItem *>("playerVariantButton");
        QVERIFY(button);
        auto *window = qobject_cast<QQuickWindow *>(root);
        QVERIFY(window);
        for (const auto &variant : {"astronaut", "pink", "robot", "classic"}) {
            const QPoint center = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center, 0);
            QCOMPARE(root->property("playerVariant").toString(), QString(variant));
            QCOMPARE(player->property("variant").toString(), QString(variant));
            QCOMPARE(game->score(), score);
            QCOMPARE(game->prizesLeft(), prizes);
            QCOMPARE(game->playerX(), x);
            QCOMPARE(game->playerY(), y);
            QCOMPARE(player->property("state").toString(), game->playerState());
            QCOMPARE(player->property("facingRight").toBool(), game->playerFacingRight());
        }
        QCOMPARE(warnings.count(), 0);
    }
};

QTEST_MAIN(TileLibraryTest)
#include "test_tilelibrary.moc"
