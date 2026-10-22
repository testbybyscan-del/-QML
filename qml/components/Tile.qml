import QtQuick 2.15

Item {
    id: tile
    width: size
    height: size
    clip: true

    property int size: 32
    property string type: "empty"
    property color crystalColor: "#35d6c4"
    // Выход становится доступен после сбора всех обязательных Призов.
    // Состояние приходит из GameEngine и обновляется вместе со счётчиком.
    property bool exitAvailable: false

    Rectangle {
        anchors.fill: parent
        visible: tile.type === "brick" || tile.type === "dug"
        color: tile.type === "brick" ? "#8b4513" : "#3a1f08"
        border.color: tile.type === "brick" ? "#5a2d0c" : "transparent"
        border.width: tile.type === "brick" ? 1 : 0

        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 4
            height: parent.height / 2 - 2
            visible: tile.type === "brick"
            color: "#a0522d"
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: tile.type === "exit"
        color: tile.exitAvailable ? "#14532d" : "#173d2c"
        border.color: tile.exitAvailable ? "#bef264" : "#4ade80"
        border.width: 2

        // Пульсирующая заливка показывает, что Персонаж выполнил условие
        // уровня и может перейти дальше, если войдёт в эту клетку.
        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            color: "#86efac"
            opacity: 0

            SequentialAnimation on opacity {
                running: tile.exitAvailable
                loops: Animation.Infinite
                NumberAnimation { from: 0.08; to: 0.48; duration: 450; easing.type: Easing.InOutSine }
                NumberAnimation { from: 0.48; to: 0.08; duration: 450; easing.type: Easing.InOutSine }
            }
        }

        Text {
            anchors.centerIn: parent
            text: "↔"
            color: "#bbf7d0"
            font.pixelSize: tile.size * 0.65
            font.bold: true
            z: 1
        }
    }

    Item {
        anchors.fill: parent
        visible: tile.type === "crystal"

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.62
            height: parent.height * 0.62
            rotation: 45
            color: tile.crystalColor
            border.color: tile.crystalColor
            border.width: 2

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 4
                width: 5
                height: 5
                color: "#e5fffc"
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: tile.type === "gold"

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.58
            height: parent.height * 0.58
            rotation: 45
            color: "#f5c542"
            border.color: "#fff0a3"
            border.width: 2

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 4
                width: 5
                height: 5
                color: "#fff8d6"
            }
        }
    }

    Item {
        id: superPrizeVisual
        anchors.fill: parent
        visible: tile.type === "superPrize"

        Rectangle {
            id: superPrizeDiamond
            anchors.centerIn: parent
            width: parent.width * 0.58
            height: parent.height * 0.58
            rotation: 45
            radius: 3
            color: "#7c3aed"
            border.color: "#e9d5ff"
            border.width: 2

            SequentialAnimation on scale {
                loops: Animation.Infinite
                NumberAnimation { from: 0.92; to: 1.04; duration: 650; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.04; to: 0.92; duration: 650; easing.type: Easing.InOutSine }
            }

            RotationAnimation on rotation {
                from: 45
                to: 405
                duration: 5000
                loops: Animation.Infinite
            }
        }

        Text {
            anchors.centerIn: parent
            text: "✦"
            color: "#fff7ed"
            font.pixelSize: tile.size * 0.48
            font.bold: true
        }
    }
}
