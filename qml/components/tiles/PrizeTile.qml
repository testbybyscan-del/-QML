import QtQuick 2.15

GameTile {
    id: root
    tileType: "gold"
    Item {
        anchors.fill: parent
        visible: !root.usesSprite
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
                anchors.margins: root.cellSize / 8
                width: root.cellSize * 5 / 32
                height: width
                color: "#fff8d6"
            }
        }
    }
}
