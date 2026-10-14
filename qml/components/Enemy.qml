import QtQuick

Item {
    id: enemy
    width: size
    height: size

    // Внешние свойства
    property int size: 32
    property string state: "run"       // "run", "climb", "fall", "trapped"
    property bool facingRight: true

    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.7
        height: parent.height * 0.7
        radius: 4
        color: enemy.state === "trapped" ? "#8b0000" : "#dc143c"
        border.color: "black"
        border.width: 1

        // "Глаз" направления
        Rectangle {
            width: 4; height: 4
            radius: 2
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            x: enemy.facingRight ? parent.width - 8 : 4
        }
    }

    // Если враг в ловушке — рисуем рамку
    Rectangle {
        anchors.fill: parent
        visible: enemy.state === "trapped"
        color: "transparent"
        border.color: "yellow"
        border.width: 2
    }
}