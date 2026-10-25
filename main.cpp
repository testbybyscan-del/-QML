#include <QGuiApplication>
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QUrl>
#include "src/gameengine.h"
#include "src/gamesettings.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Majorov"));
    QCoreApplication::setApplicationName(QStringLiteral("BoulderDash"));
    qmlRegisterType<GameEngine>("BoulderDash", 1, 0, "GameEngine");
    qmlRegisterType<GameSettings>("BoulderDash", 1, 0, "GameSettings");
    QQmlApplicationEngine engine;
    const QUrl url(app.arguments().contains(QStringLiteral("--tile-gallery"))
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
    if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()))
        window->show();
    return app.exec();
}
