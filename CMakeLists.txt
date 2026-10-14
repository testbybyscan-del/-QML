cmake_minimum_required(VERSION 3.21)

project(loderunner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматическая обработка moc/uic/rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 COMPONENTS Quick Gui Qml REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

# Точка входа приложения
qt_add_executable(loderunner
    main.cpp
    src/gameengine.cpp
    src/gameengine.h
)

# QML-модуль: связывает C++ и QML
qt_add_qml_module(loderunner
    URI LodeRunner
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/components/Player.qml
        qml/components/Enemy.qml
        qml/components/Tile.qml
    RESOURCES
        assets/images/player.png
        assets/images/enemy.png
        assets/images/tiles.png
)

# Линковка с Qt-модулями
target_link_libraries(loderunner PRIVATE
    Qt6::Quick
    Qt6::Gui
    Qt6::Qml
)