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

    readonly property int tileSize: 32

    Levels.Level1 { id: level1 }
    Levels.Level2 { id: level2 }
    Levels.Level3 { id: level3 }
    Levels.Level4 { id: level4 }
    Levels.Level5 { id: level5 }

    GameEngine {
        id: gameEngine
    }

    Timer {
        interval: 250
        running: true
        repeat: true
        onTriggered: gameEngine.updateCrystals()
    }

    Item {
        id: gameRoot
        anchors.fill: parent
        focus: true
        Keys.onPressed: (event) => {
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
            case Qt.Key_Escape:
                Qt.quit()
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
            spacing: 20

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
                        type: model.type
                        hasKey: gameEngine.hasKey
                        safeAlert: model.type === "safe" && !gameEngine.hasKey
                                   && Math.abs(gameEngine.playerX - model.col)
                                      + Math.abs(gameEngine.playerY - model.row) === 1
                        exitAvailable: gameEngine.prizesLeft === 0
                        crystalColor: gameEngine.currentLevel === 2
                                      ? "#db4f9b"
                                      : gameEngine.currentLevel === 3 ? "#38bdf8"
                                      : gameEngine.currentLevel === 4 ? "#f59e0b"
                                      : gameEngine.currentLevel > gameEngine.authoredLevelCount
                                        ? "#c084fc" : "#35d6c4"
                    }
                }

                Player {
                    x: gameEngine.playerX * root.tileSize
                    y: gameEngine.playerY * root.tileSize
                    size: root.tileSize
                    state: gameEngine.playerState
                    facingRight: gameEngine.playerFacingRight
                }
            }

            Text {
                width: board.width
                text: gameEngine.currentLevel === 2
                      ? gameEngine.hasKey
                        ? qsTr("Ключ найден. Откройте Сейф B на этом уровне (строка 8, столбец 10).")
                        : qsTr("На этом уровне Сейф B (строка 8, столбец 10). Ключ находится на уровне 4.")
                      : gameEngine.currentLevel === 4
                        ? gameEngine.hasKey
                          ? qsTr("Ключ подобран. Вернитесь через левый выход на уровень 2 к Сейфу.")
                          : qsTr("Подберите Ключ K (строка 10, столбец 15), затем вернитесь на уровень 2 к Сейфу.")
                        : gameEngine.prizesLeft === 0
                      ? qsTr("Все Призы собраны. Дойдите до выхода в правом нижнем углу, чтобы продолжить. Нижний левый угол — назад")
                      : gameEngine.currentLevel > 1
                        ? qsTr("Соберите все призы: стрелки/WASD — движение, пробел — копать. Нижний левый угол — предыдущий экран, нижний правый — следующий после сбора всех призов")
                        : qsTr("Соберите все Призы: стрелки/WASD — движение, пробел — копать, Esc — выход")
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
                        font.pixelSize: 30
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
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
                        font.pixelSize: 17
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
                            font.pixelSize: 18
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
                            font.pixelSize: 17
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
                width: Math.min(parent.width - 40, 410)
                height: 220
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
                        font.pixelSize: 23
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Найдите Ключ на четвёртом уровне и вернитесь к Сейфу.")
                        color: "#cbd5e1"
                        font.pixelSize: 17
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
                            font.pixelSize: 18
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

        Component.onCompleted: {
            gameEngine.initializeLevels([
                level1.rows, level2.rows, level3.rows, level4.rows, level5.rows
            ])
            forceActiveFocus()
        }
        onActiveFocusChanged: if (activeFocus) forceActiveFocus()
    }
}
