#include <QGuiApplication>
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QUrl>
#include <QCommandLineParser>
#include <QDebug>
#include "src/gameengine.h"
#include "src/gamesettings.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Majorov"));
    QCoreApplication::setApplicationName(QStringLiteral("BoulderDash"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Boulder Dash"));
    parser.addHelpOption();
    const QCommandLineOption galleryOption(QStringLiteral("tile-gallery"),
                                          QStringLiteral("Открыть галерею тайлов."));
    const QCommandLineOption levelOption(QStringLiteral("level"),
                                        QStringLiteral("Начать игру с уровня 1–50."),
                                        QStringLiteral("N"));
    parser.addOption(galleryOption);
    parser.addOption(levelOption);
    parser.process(app);
    int startLevel = 0;
    if (!parser.positionalArguments().isEmpty()) {
        qCritical().noquote() << "Неожиданные позиционные аргументы. Используйте --help.";
        return 1;
    }
    if (parser.isSet(levelOption)) {
        bool ok = false;
        startLevel = parser.value(levelOption).toInt(&ok);
        if (!ok || startLevel < 1 || startLevel > 50 || parser.values(levelOption).size() != 1) {
            qCritical().noquote() << "Параметр --level должен задавать одно целое число от 1 до 50.";
            return 1;
        }
        if (parser.isSet(galleryOption)) {
            qCritical().noquote() << "Параметры --level и --tile-gallery несовместимы.";
            return 1;
        }
    }
    qmlRegisterType<GameEngine>("BoulderDash", 1, 0, "GameEngine");
    qmlRegisterType<GameSettings>("BoulderDash", 1, 0, "GameSettings");
    QQmlApplicationEngine engine;
    const QUrl url(parser.isSet(galleryOption)
                   ? QStringLiteral("qrc:/examples/TileGallery.qml")
                   : QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);
    if (engine.rootObjects().isEmpty())
        return -1;
    if (startLevel != 0) {
        QObject *root = engine.rootObjects().first();
        auto *game = root->findChild<GameEngine *>();
        if (!game || !game->startAtLevel(startLevel)) {
            qCritical().noquote() << "Не удалось загрузить начальный уровень" << startLevel;
            return 1;
        }
        root->setProperty("sessionStarted", true);
        if (!QMetaObject::invokeMethod(root, "resumeGame"))
            return 1;
        qInfo().noquote() << "Начальный уровень:" << game->currentLevel()
                          << "Авторских уровней:" << game->authoredLevelCount();
    }
    if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()))
        window->show();
    return app.exec();
}
