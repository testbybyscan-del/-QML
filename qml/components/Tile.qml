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
}
