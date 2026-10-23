cmake_minimum_required(VERSION 3.16)

project(boulderdash VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматическая обработка moc/uic/rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.15 COMPONENTS Core Quick Gui Qml REQUIRED)

include(CTest)

qt5_add_resources(QML_RESOURCES qml.qrc)

# Точка входа приложения
add_executable(boulderdash
    main.cpp
    src/gameengine.cpp
    src/gameengine.h
    src/landscapegenerator.cpp
    src/landscapegenerator.h
    ${QML_RESOURCES}
)

# Линковка с Qt-модулями
target_link_libraries(boulderdash PRIVATE
    Qt5::Core
    Qt5::Quick
    Qt5::Gui
    Qt5::Qml
)

if(BUILD_TESTING)
    find_package(Qt5 5.15 COMPONENTS Test REQUIRED)

    add_executable(test_gameengine
        tests/test_gameengine.cpp
        src/gameengine.cpp
        src/gameengine.h
        src/landscapegenerator.cpp
        src/landscapegenerator.h
        ${QML_RESOURCES}
    )

    target_link_libraries(test_gameengine PRIVATE
        Qt5::Core
        Qt5::Qml
        Qt5::Test
    )

    add_test(NAME gameengine COMMAND test_gameengine)
endif()
