import QtQuick 2.15

GameTile {
    id: root
    tileType: "brick"
    Rectangle {
        anchors.fill: parent
        visible: !root.usesSprite
        color: "#8b4513"
        border.color: "#5a2d0c"
        border.width: 1
        Rectangle {
            anchors.centerIn: parent
            width: Math.max(0, parent.width - 4)
            height: Math.max(0, parent.height / 2 - 2)
            color: "#a0522d"
        }
    }
}
