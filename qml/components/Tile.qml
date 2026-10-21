import QtQuick 2.15

Item {
    id: tile
    width: size
    height: size

    property int size: 32
    property string type: "empty"

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

    Item {
        anchors.fill: parent
        visible: tile.type === "crystal"

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.62
            height: parent.height * 0.62
            rotation: 45
            color: "#35d6c4"
            border.color: "#b7fff6"
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
