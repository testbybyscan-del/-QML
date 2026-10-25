import QtQuick 2.15

Rectangle {
    id: root
    property bool sessionStarted: false
    signal resumeRequested()
    signal newGameRequested()
    signal settingsRequested()
    signal quitRequested()
    color: "#ed0f172a"
    MouseArea { anchors.fill: parent } // Не пропускать клики на поле под меню.
    Column {
        anchors.centerIn: parent
        width: Math.min(360, root.width - 40)
        spacing: 14
        Text {
            width: parent.width
            text: root.sessionStarted ? qsTr("Пауза") : qsTr("Boulder Dash")
            color: "#f8fafc"; font.pixelSize: 30; font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            width: parent.width
            text: root.sessionStarted ? qsTr("Esc или P — продолжить") : qsTr("Выберите оформление и начните игру")
            color: "#94a3b8"; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter
        }
        MenuButton {
            objectName: "resumeButton"
            width: parent.width
            visible: root.sessionStarted
            text: qsTr("Продолжить")
            onClicked: root.resumeRequested()
        }
        MenuButton {
            objectName: "newGameButton"
            width: parent.width
            text: root.sessionStarted ? qsTr("Новая игра") : qsTr("Начать игру")
            onClicked: root.newGameRequested()
        }
        MenuButton {
            objectName: "openSettingsButton"
            width: parent.width; text: qsTr("Настройки")
            onClicked: root.settingsRequested()
        }
        MenuButton {
            width: parent.width; text: qsTr("Выход")
            onClicked: root.quitRequested()
        }
    }
}
