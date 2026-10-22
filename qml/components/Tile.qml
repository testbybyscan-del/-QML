import QtQuick 2.15

Item {
    id: tile
    width: size
    height: size

    property int size: 32
    property string type: "empty"
    property color crystalColor: "#35d6c4"

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
        color: "#173d2c"
        border.color: "#4ade80"
        border.width: 2

        Text {
            anchors.centerIn: parent
            text: "↔"
            color: "#bbf7d0"
            font.pixelSize: tile.size * 0.65
            font.bold: true
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
}
