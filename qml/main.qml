import QtQuick 2.15
import QtQuick.Window 2.15
import "components"
import "levels" as Levels
import BoulderDash 1.0

Window {
    id: root
    visible: true
    width: 800
    height: 600
    minimumWidth: 680
    minimumHeight: 520
    title: qsTr("Boulder Dash")
    color: "#111827"

    property alias playerVariant: settings.playerVariant
    property alias tileTheme: settings.tileTheme
    property bool sessionStarted: false
    property string overlay: "menu"
    property string settingsReturn: "menu"
    readonly property GameSettings gameSettingsObject: settings
    GameSettings { id: settings; objectName: "gameSettings" }
    function resumeGame() {
        if (sessionStarted) overlay = ""
        gameRoot.forceActiveFocus()
    }
    function newGame() {
        gameEngine.restartGame()
        sessionStarted = true
        overlay = ""
        gameRoot.forceActiveFocus()
    }
    function openSettings() {
        settingsReturn = overlay === "menu" ? "menu" : ""
        overlay = "settings"
    }
    function togglePause() {
        if (overlay === "settings") overlay = settingsReturn
        else if (overlay === "menu" && sessionStarted) resumeGame()
        else overlay = "menu"
    }
    Shortcut { sequence: "Escape"; onActivated: root.togglePause() }
    Shortcut { sequence: "P"; onActivated: root.togglePause() }
    PlayerStyles { id: playerStyles }
    readonly property int tileSize: 32
    readonly property string messageFontFamily: zxSpectrumFont.status === FontLoader.Ready
                                                ? zxSpectrumFont.name : "monospace"

    FontLoader {
        id: zxSpectrumFont
        source: "qrc:/fonts/Unifont.otf"
    }

    Levels.Level1 { id: level1 }
    Levels.Level2 { id: level2 }
    Levels.Level3 { id: level3 }
    Levels.Level4 { id: level4 }
    Levels.Level5 { id: level5 }
    Levels.Level6 { id: level6 }

    Loader {
        id: galleryLoader
        active: false
        source: "examples/TileGallery.qml"
    }

    GameEngine {
        id: gameEngine
        paused: root.overlay !== ""
    }

    Timer {
        objectName: "physicsTimer"
        interval: 250
        running: !gameEngine.paused
        repeat: true
        onTriggered: gameEngine.updateCrystals()
    }

    Item {
        id: gameRoot
        anchors.fill: parent
        focus: true
        Keys.onPressed: (event) => {
            if (root.overlay !== "") { event.accepted = true; return }
            switch (event.key) {
            case Qt.Key_Left:
            case Qt.Key_A:
                gameEngine.movePlayer(-1, 0)
                break
            case Qt.Key_Right:
            case Qt.Key_D:
                gameEngine.movePlayer(1, 0)
                break
            case Qt.Key_Up:
            case Qt.Key_W:
                gameEngine.movePlayer(0, -1)
                break
            case Qt.Key_Down:
            case Qt.Key_S:
                gameEngine.movePlayer(0, 1)
                break
            case Qt.Key_Space:
                gameEngine.dig()
                break
            default:
                return
            }
            event.accepted = true
        }

        Rectangle {
            anchors.fill: parent
            color: "#111827"
        }

        Column {
            anchors.centerIn: parent
            scale: Math.min(1, (gameRoot.width - 32) / width, (gameRoot.height - 32) / height)
            spacing: 20

            Flow {
                width: board.width
                spacing: 12
                MenuButton {
                    objectName: "pauseButton"
                    width: 108; height: 30; text: qsTr("Пауза")
                    onClicked: root.overlay = "menu"
                }
                MenuButton {
                    objectName: "toolbarSettingsButton"
                    width: 108; height: 30; text: qsTr("Настройки")
                    onClicked: root.openSettings()
                }
                Rectangle {
                    objectName: "playerVariantButton"
                    width: 236; height: 30; radius: 5; color: "#334155"
                    Text { anchors.centerIn: parent; text: qsTr("Герой: %1").arg(playerStyles.style(root.playerVariant).label); color: "#f8fafc"; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.playerVariant = playerStyles.next(root.playerVariant)
                            gameRoot.forceActiveFocus()
                        }
                    }
                }
                Rectangle {
                    width: 196; height: 30; radius: 5; color: "#334155"
                    Text { anchors.centerIn: parent; text: qsTr("Тема: %1").arg(root.tileTheme); color: "#f8fafc" }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.tileTheme = root.tileTheme === "classic" ? "kenney"
                                           : root.tileTheme === "kenney" ? "industrial" : "classic"
                            gameRoot.forceActiveFocus()
                        }
                    }
                }
                Rectangle {
                    width: 158; height: 30; radius: 5; color: "#334155"
                    Text { anchors.centerIn: parent; text: qsTr("Галерея тайлов"); color: "#f8fafc" }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (!galleryLoader.active) galleryLoader.active = true
                            else { galleryLoader.item.show(); galleryLoader.item.raise() }
                        }
                    }
                }
            }

            Row {
                width: board.width
                spacing: 0

                Text {
                    width: parent.width / 2
                    text: gameEngine.currentLevel > gameEngine.authoredLevelCount
                          ? qsTr("ПЕЙЗАЖ %1").arg(
                                gameEngine.currentLevel - gameEngine.authoredLevelCount)
                          : qsTr("BOULDER DASH — УРОВЕНЬ %1").arg(gameEngine.currentLevel)
                    color: "#f3f4f6"
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    width: parent.width / 2
                    text: qsTr("Призы: %1   Очки: %2\nРекорд: %3   Жизни: %4   Ключ: %5")
                          .arg(gameEngine.prizesLeft)
                          .arg(gameEngine.score)
                          .arg(gameEngine.highScore)
                          .arg(gameEngine.lives)
                          .arg(gameEngine.hasKey ? "есть" : "нет")
                    color: "#d1d5db"
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Rectangle {
                id: board
                width: gameEngine.columns * root.tileSize
                height: gameEngine.rows * root.tileSize
                color: "#172033"
                border.color: "#526078"
                border.width: 2
                clip: true

                Repeater {
                    model: gameEngine.tilesModel
                    delegate: Tile {
                        x: model.col * root.tileSize
                        y: model.row * root.tileSize
                        size: root.tileSize
                        theme: root.tileTheme
                        animated: settings.animationsEnabled && !gameEngine.paused
                        animationInterval: settings.animationInterval
                        type: model.type
                        hasKey: gameEngine.hasKey
                        safeAlert: model.type === "safe" && !gameEngine.hasKey
                                   && Math.abs(gameEngine.playerX - model.col)
                                      + Math.abs(gameEngine.playerY - model.row) === 1
                        exitAvailable: model.row === gameEngine.rows - 2 && model.col === 1
                                       ? gameEngine.canGoBack : gameEngine.prizesLeft === 0
                        crystalColor: gameEngine.currentLevel === 2
                                      ? "#db4f9b"
                                      : gameEngine.currentLevel === 3 ? "#38bdf8"
                                      : gameEngine.currentLevel === 4 ? "#f59e0b"
                                      : gameEngine.currentLevel > gameEngine.authoredLevelCount
                                        ? "#c084fc" : "#35d6c4"
                    }
                }

                Player {
                    objectName: "gamePlayer"
                    variant: root.playerVariant
                    animated: settings.animationsEnabled && !gameEngine.paused
                    animationInterval: settings.animationInterval
                    x: gameEngine.playerX * root.tileSize
                    y: gameEngine.playerY * root.tileSize
                    size: root.tileSize
                    state: gameEngine.playerState
                    facingRight: gameEngine.playerFacingRight
                }
            }

            Text {
                width: board.width
                text: gameEngine.currentLevel > 1 && !gameEngine.canGoBack
                      ? gameEngine.prizesLeft === 0
                        ? qsTr("Все Призы собраны. Правый выход ведёт дальше. Возврат ниже стартового уровня теста недоступен.")
                        : qsTr("Соберите все Призы, чтобы открыть правый выход. Возврат ниже стартового уровня теста недоступен.")
                      : gameEngine.currentLevel === 2
                      ? gameEngine.hasKey
                        ? qsTr("Ключ найден. Откройте Сейф B на этом уровне (строка 8, столбец 10).")
                        : qsTr("На этом уровне Сейф B (строка 8, столбец 10). Ключ находится на уровне 4.")
                      : gameEngine.currentLevel === 4
                        ? gameEngine.hasKey
                          ? qsTr("Ключ подобран. Вернитесь через левый выход на уровень 2 к Сейфу.")
                          : qsTr("Подберите Ключ K (строка 10, столбец 15), затем вернитесь на уровень 2 к Сейфу.")
                        : gameEngine.prizesLeft === 0
                      ? gameEngine.canGoBack
                        ? qsTr("Все Призы собраны. Дойдите до выхода в правом нижнем углу, чтобы продолжить. Нижний левый угол — назад")
                        : qsTr("Все Призы собраны. Дойдите до выхода в правом нижнем углу, чтобы продолжить.")
                      : gameEngine.canGoBack
                        ? qsTr("Соберите все призы: стрелки/WASD — движение, пробел — копать. Нижний левый угол — предыдущий экран, нижний правый — следующий после сбора всех призов")
                        : qsTr("Соберите все Призы: стрелки/WASD — движение, пробел — копать, Esc/P — пауза")
                color: "#cbd5e1"
                font.pixelSize: 15
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: gameEngine.gameOver
            color: "#b3000000"
            z: 100

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 40, 460)
                height: 390
                radius: 14
                color: "#1e293b"
                border.color: "#64748b"
                border.width: 2

                Column {
                    anchors.fill: parent
                    anchors.margins: 26
                    spacing: 14

                    Text {
                        width: parent.width
                        text: qsTr("Игра приостановлена")
                        color: "#f8fafc"
                        font.family: root.messageFontFamily
                        font.pixelSize: 21
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Очки: %1\nРекорд: %2\nОсталось жизней: %3\nУровень: %4\nОбязательных Призов осталось: %5\nКлюч: %6")
                              .arg(gameEngine.score)
                              .arg(gameEngine.highScore)
                              .arg(gameEngine.lives)
                              .arg(gameEngine.currentLevel)
                              .arg(gameEngine.prizesLeft)
                              .arg(gameEngine.hasKey ? qsTr("есть") : qsTr("нет"))
                        color: "#e2e8f0"
                        font.family: root.messageFontFamily
                        font.pixelSize: 14
                        lineHeight: 1.25
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Rectangle {
                        width: parent.width
                        height: 46
                        radius: 8
                        color: restartMouse.containsMouse ? "#2563eb" : "#1d4ed8"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Перезапустить игру")
                            color: "white"
                            font.family: root.messageFontFamily
                            font.pixelSize: 14
                            font.bold: true
                        }

                        MouseArea {
                            id: restartMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: gameEngine.restartGame()
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 42
                        radius: 8
                        color: finishMouse.containsMouse ? "#475569" : "#334155"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Закончить игру")
                            color: "#f8fafc"
                            font.family: root.messageFontFamily
                            font.pixelSize: 14
                        }

                        MouseArea {
                            id: finishMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.quit()
                        }
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: gameEngine.keyRequiredNotice && !gameEngine.gameOver
            color: "#990f172a"
            z: 101

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 40, 430)
                height: 260
                radius: 14
                color: "#1e293b"
                border.color: "#facc15"
                border.width: 2

                Column {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 18

                    Text {
                        width: parent.width
                        text: qsTr("Для открытия Сейфа нужен Ключ")
                        color: "#f8fafc"
                        font.family: root.messageFontFamily
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Найдите Ключ на четвёртом уровне и вернитесь к Сейфу.")
                        color: "#cbd5e1"
                        font.family: root.messageFontFamily
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        width: parent.width
                        height: 44
                        radius: 8
                        color: acknowledgeMouse.containsMouse ? "#b45309" : "#92400e"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Понятно")
                            color: "#fff7ed"
                            font.family: root.messageFontFamily
                            font.pixelSize: 14
                            font.bold: true
                        }

                        MouseArea {
                            id: acknowledgeMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: gameEngine.dismissKeyRequiredNotice()
                        }
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: gameEngine.keyCollectedNotice && !gameEngine.gameOver
            color: "#990f172a"
            z: 102

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 40, 430)
                height: 250
                radius: 14
                color: "#1e293b"
                border.color: "#34d399"
                border.width: 2

                Column {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 18

                    Text {
                        width: parent.width
                        text: qsTr("Ключ найден!")
                        color: "#f8fafc"
                        font.family: root.messageFontFamily
                        font.pixelSize: 21
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Ключ у Персонажа. Вернитесь к Сейфу, чтобы открыть его.")
                        color: "#cbd5e1"
                        font.family: root.messageFontFamily
                        font.pixelSize: 15
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        width: parent.width
                        height: 44
                        radius: 8
                        color: keyCollectedMouse.containsMouse ? "#047857" : "#065f46"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Продолжить")
                            color: "#ecfdf5"
                            font.family: root.messageFontFamily
                            font.pixelSize: 14
                            font.bold: true
                        }

                        MouseArea {
                            id: keyCollectedMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: gameEngine.dismissKeyCollectedNotice()
                        }
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: gameEngine.safeOpenedNotice && !gameEngine.gameOver
            color: "#990f172a"
            z: 103

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 40, 430)
                height: 250
                radius: 14
                color: "#1e293b"
                border.color: "#facc15"
                border.width: 2

                Column {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 18

                    Text {
                        width: parent.width
                        text: qsTr("Сейф открыт!")
                        color: "#f8fafc"
                        font.family: root.messageFontFamily
                        font.pixelSize: 21
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Ключ использован. За открытие начислено 100 очков.")
                        color: "#cbd5e1"
                        font.family: root.messageFontFamily
                        font.pixelSize: 15
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        width: parent.width
                        height: 44
                        radius: 8
                        color: safeOpenedMouse.containsMouse ? "#b45309" : "#92400e"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Продолжить")
                            color: "#fff7ed"
                            font.family: root.messageFontFamily
                            font.pixelSize: 14
                            font.bold: true
                        }

                        MouseArea {
                            id: safeOpenedMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: gameEngine.dismissSafeOpenedNotice()
                        }
                    }
                }
            }
        }

        Component.onCompleted: {
            gameEngine.initializeLevels([
                level1.rows, level2.rows, level3.rows, level4.rows, level5.rows, level6.rows
            ])
            forceActiveFocus()
        }

    }
    GameMenu {
        anchors.fill: parent
        z: 200
        visible: root.overlay === "menu"
        sessionStarted: root.sessionStarted
        onResumeRequested: root.resumeGame()
        onNewGameRequested: root.newGame()
        onSettingsRequested: root.openSettings()
        onQuitRequested: Qt.quit()
    }
    SettingsPanel {
        anchors.fill: parent
        z: 201
        visible: root.overlay === "settings"
        settings: root.gameSettingsObject
        onClosed: {
            root.overlay = root.settingsReturn
            gameRoot.forceActiveFocus()
        }
    }

}
