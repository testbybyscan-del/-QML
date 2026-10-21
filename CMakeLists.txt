cmake_minimum_required(VERSION 3.16)

project(loderunner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматическая обработка moc/uic/rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.15 COMPONENTS Core Quick Gui Qml REQUIRED)

qt5_add_resources(QML_RESOURCES qml.qrc)

# Точка входа приложения
add_executable(loderunner
    main.cpp
    src/gameengine.cpp
    src/gameengine.h
    ${QML_RESOURCES}
)

# Линковка с Qt-модулями
target_link_libraries(loderunner PRIVATE
    Qt5::Core
    Qt5::Quick
    Qt5::Gui
    Qt5::Qml
)
