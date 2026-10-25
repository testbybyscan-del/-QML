import QtQuick 2.15
import BoulderDash 1.0
import "tiles" as Tiles

Rectangle {
    id: root
    property GameSettings settings
    signal closed()
    color: "#f50f172a"
    PlayerStyles { id: styles }
    MouseArea { anchors.fill: parent }
    Flickable {
        anchors { fill: parent; margins: 24 }
        clip: true
        contentWidth: width
        contentHeight: content.height
        Column {
            id: content
            width: Math.min(480, parent.width)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10
            Text { text: qsTr("Настройки оформления"); color: "#f8fafc"; font.pixelSize: 25; font.bold: true }
            Text {
                width: parent.width
                text: qsTr("Изменения сохраняются автоматически. Игра остаётся на паузе.")
                wrapMode: Text.WordWrap; color: "#94a3b8"
            }
            Row {
                height: 72; spacing: 22
                Player {
                    objectName: "settingsPlayerPreview"
                    size: 64; variant: root.settings.playerVariant; state: "run"
                    animated: root.settings.animationsEnabled
                    animationInterval: root.settings.animationInterval
                }
                Tiles.BrickTile { cellSize: 48; theme: root.settings.tileTheme }
                Tiles.CrystalTile { cellSize: 48; theme: root.settings.tileTheme }
                Tiles.PrizeTile {
                    cellSize: 48; theme: root.settings.tileTheme
                    animated: root.settings.animationsEnabled
                    animationInterval: root.settings.animationInterval
                }
            }
            MenuButton {
                objectName: "settingsThemeButton"
                width: parent.width; text: qsTr("Тема: %1").arg(root.settings.tileTheme)
                onClicked: root.settings.tileTheme = root.settings.tileTheme === "classic" ? "kenney"
                                                  : root.settings.tileTheme === "kenney" ? "industrial" : "classic"
            }
            MenuButton {
                objectName: "settingsPlayerButton"
                width: parent.width; text: qsTr("Герой: %1").arg(styles.style(root.settings.playerVariant).label)
                onClicked: root.settings.playerVariant = styles.next(root.settings.playerVariant)
            }
            MenuButton {
                objectName: "settingsAnimationsButton"
                width: parent.width; text: qsTr("Анимация: %1").arg(root.settings.animationsEnabled ? "включена" : "выключена")
                onClicked: root.settings.animationsEnabled = !root.settings.animationsEnabled
            }
            MenuButton {
                objectName: "settingsSpeedButton"
                width: parent.width
                text: qsTr("Скорость анимации: %1 мс / кадр").arg(root.settings.animationInterval)
                onClicked: root.settings.animationInterval = root.settings.animationInterval === 80 ? 140
                                                          : root.settings.animationInterval === 140 ? 220 : 80
            }
            Text {
                width: parent.width
                text: qsTr("Меньше интервал — быстрее анимация. Скорость игры не меняется.")
                color: "#94a3b8"; wrapMode: Text.WordWrap
            }
            MenuButton {
                objectName: "resetSettingsButton"
                width: parent.width; text: qsTr("Сбросить оформление")
                onClicked: root.settings.resetDefaults()
            }
            MenuButton {
                objectName: "closeSettingsButton"
                width: parent.width; text: qsTr("Назад")
                onClicked: root.closed()
            }
        }
    }
}
