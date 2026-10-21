import QtQuick 2.15
import QtQuick.Window 2.15
import "components"
import LodeRunner 1.0

Window {
    id: root
    visible: true
    width: 800
    height: 600
    minimumWidth: 680
    minimumHeight: 520
    title: qsTr("Lode Runner")
    color: "#111827"

    readonly property int tileSize: 32

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
                    text: qsTr("LODE RUNNER — УРОВЕНЬ %1").arg(gameEngine.currentLevel)
                    color: "#f3f4f6"
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    width: parent.width / 2
                    text: qsTr("Призы: %1   Очки: %2   Жизни: %3")
                          .arg(gameEngine.prizesLeft)
                          .arg(gameEngine.score)
                          .arg(gameEngine.lives)
                    color: "#d1d5db"
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignRight
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
                        crystalColor: gameEngine.currentLevel === 2 ? "#db4f9b" : "#35d6c4"
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
                      ? qsTr("Соберите все призы: стрелки/WASD — движение, пробел — копать. Нижний левый угол — возврат на первый уровень, Esc — выход")
                      : qsTr("Соберите все призы: стрелки/WASD — движение, пробел — копать, Esc — выход")
                color: "#cbd5e1"
                font.pixelSize: 15
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: gameEngine.gameOver || gameEngine.levelComplete
            color: "#b3000000"

            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 40
                font.bold: true
                text: gameEngine.levelComplete ? qsTr("Уровень пройден!") : qsTr("Игра окончена")
            }
        }

        Component.onCompleted: forceActiveFocus()
        onActiveFocusChanged: if (activeFocus) forceActiveFocus()
    }
}
