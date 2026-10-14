#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>

// Если у вас есть C++ классы, которые нужно использовать в QML
#include "src/gameengine.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Регистрируем C++ тип для использования в QML
    qmlRegisterType<GameEngine>("LodeRunner", 1, 0, "GameEngine");

    QQmlApplicationEngine engine;

    // Загрузка QML из модуля (Qt 6.5+)
    engine.loadFromModule("LodeRunner", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}