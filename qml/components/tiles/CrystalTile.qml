import QtQuick 2.15

GameTile {
    id: root
    tileType: "crystal"
    property color crystalColor: "#35d6c4"
    Item {
        anchors.fill: parent
        visible: !root.usesSprite
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.62
            height: parent.height * 0.62
            rotation: 45
            color: root.crystalColor
            border.color: root.crystalColor
            border.width: 2
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: root.cellSize / 8
                width: root.cellSize * 5 / 32
                height: width
                color: "#e5fffc"
            }
        }
    }
}
