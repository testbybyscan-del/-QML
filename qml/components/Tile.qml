
import QtQuick 2.15

Item {
    id: tile
    width: size
    height: size

    // Внешние свойства
    property int size: 32
    property string type: "empty" // "brick", "ladder", "rope", "gold", "empty", "dug"

    // Кирпич
    Rectangle {
        anchors.fill: parent
        visible: tile.type === "brick"
        color: "#8B4513"
        border.color: "#5a2d0c"
        border.width: 1
        // Простая текстура "кирпича"
        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 4
            height: parent.height / 2 - 2
            color: "#a0522d"
        }
    }

    // Лестница
    Item {
        anchors.fill: parent
        visible: tile.type === "ladder"
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 4; height: parent.height; color: "#deb887" }
        Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 4; color: "#deb887"; anchors.horizontalCenter: parent.horizontalCenter }
        Rectangle { anchors.left: parent.left; width: 4; height: parent.height; color: "#deb887" }
        Rectangle { anchors.right: parent.right; width: 4; height: parent.height; color: "#deb887" }
    }

    // Веревка
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: tile.type === "rope"
        width: 4
        height: parent.height
        color: "#c0c0c0"
    }

    // Золото
    Item {
        anchors.fill: parent
        visible: tile.type === "gold"
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.5
            height: parent.height * 0.5
            radius: width / 2
            color: "gold"
            border.color: "#b8860b"
            border.width: 2
        }
    }

    // Вырытая яма (кирпич исчез, но не полностью)
    Rectangle {
        anchors.fill: parent
        visible: tile.type === "dug"
        color: "#3a1f08"
    }
}
