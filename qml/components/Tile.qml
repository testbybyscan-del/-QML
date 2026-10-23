import QtQuick 2.15

Item {
    id: tile
    width: size
    height: size
    clip: true

    property int size: 32
    property string type: "empty"
    property color crystalColor: "#35d6c4"
    property bool hasKey: false
    property bool safeAlert: false
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

    // Запертый Сейф — отдельная статичная клетка, не участник физики.
    Item {
        anchors.fill: parent
        visible: tile.type === "safe"

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.76
            height: parent.height * 0.68
            radius: 4
            color: "#475569"
            border.color: tile.hasKey ? "#facc15" : "#94a3b8"
            border.width: 2

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.4
                height: width
                radius: width / 2
                color: "#cbd5e1"

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 0.42
                    height: width
                    radius: width / 2
                    color: "#1e293b"
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            border.color: "#facc15"
            border.width: 3
            opacity: 0
            visible: tile.safeAlert

            SequentialAnimation on opacity {
                running: tile.safeAlert
                loops: Animation.Infinite
                NumberAnimation { from: 0.05; to: 0.95; duration: 350; easing.type: Easing.InOutSine }
                NumberAnimation { from: 0.95; to: 0.05; duration: 350; easing.type: Easing.InOutSine }
            }
        }

        Text {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 2
            text: "С"
            color: "#f8fafc"
            font.pixelSize: tile.size * 0.34
            font.bold: true
            z: 1
        }
    }

    // Ключ собирается при входе в клетку и переносится между уровнями.
    Item {
        anchors.fill: parent
        visible: tile.type === "key"

        Rectangle {
            x: parent.width * 0.45
            y: parent.height * 0.47
            width: parent.width * 0.43
            height: parent.height * 0.13
            rotation: -35
            radius: height / 2
            color: "#facc15"
        }

        Rectangle {
            x: parent.width * 0.14
            y: parent.height * 0.23
            width: parent.width * 0.4
            height: width
            radius: width / 2
            color: "#facc15"

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.42
                height: width
                radius: width / 2
                color: "#172033"
            }
        }

        Rectangle {
            x: parent.width * 0.65
            y: parent.height * 0.57
            width: parent.width * 0.1
            height: parent.height * 0.2
            color: "#facc15"
        }

        Text {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 1
            text: "К"
            color: "#fff7cc"
            font.pixelSize: tile.size * 0.34
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
        anchors.fill: parent
        visible: tile.type === "bonusPrize"

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.58
            height: parent.height * 0.58
            rotation: 45
            color: "#d97706"
            border.color: "#fde68a"
            border.width: 2

            Text {
                anchors.centerIn: parent
                rotation: -45
                text: "+"
                color: "#fff7cc"
                font.pixelSize: tile.size * 0.5
                font.bold: true
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
