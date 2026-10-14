import QtQuick
import QtQuick.Window
import LodeRunner 1.0

Window {
    id: root
    visible: true
    width: 800
    height: 600
    title: qsTr("Lode Runner")
    color: "#000000"

    // Размер одного тайла в пикселях
    readonly property int tileSize: 32

    // Экземпляр C++ игровой логики
    GameEngine {
        id: gameEngine
    }

    // Игровой цикл (~60 FPS)
    Timer {
        id: gameLoop
        interval: 16
        running: true
        repeat: true
        onTriggered: gameEngine.updateGame()
    }

    // Корневой контейнер для приёма ввода
    Item {
        id: gameRoot
        anchors.fill: parent
        focus: true

        // Обработка клавиатуры
        Keys.onPressed: (event) => {
            switch (event.key) {
            case Qt.Key_Left:
                gameEngine.movePlayer(-1, 0); break
            case Qt.Key_Right:
                gameEngine.movePlayer(1, 0); break
            case Qt.Key_Up:
                gameEngine.movePlayer(0, -1); break
            case Qt.Key_Down:
                gameEngine.movePlayer(0, 1); break
            case Qt.Key_Space:
                gameEngine.dig(); break
            case Qt.Key_Escape:
                Qt.quit(); break
            default:
                return
            }
            event.accepted = true
        }

        // Игровое поле (тайлы уровня)
        Item {
            id: board
            anchors.centerIn: parent
            width: gameEngine.columns * root.tileSize
            height: gameEngine.rows * root.tileSize

            // Отрисовка тайлов через Repeater
            Repeater {
                model: gameEngine.tilesModel
                delegate: Tile {
                    x: model.col * root.tileSize
                    y: model.row * root.tileSize
                    size: root.tileSize
                    type: model.type
                }
            }
        }

        // Игрок
        Player {
            id: player
            x: gameEngine.playerX * root.tileSize
            y: gameEngine.playerY * root.tileSize
            size: root.tileSize
            state: gameEngine.playerState
            facingRight: gameEngine.playerFacingRight
        }

        // Враги
        Repeater {
            model: gameEngine.enemiesModel
            delegate: Enemy {
                x: model.x * root.tileSize
                y: model.y * root.tileSize
                size: root.tileSize
                state: model.state
                facingRight: model.facingRight
            }
        }

        // HUD
        Text {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 8
            color: "white"
            font.pixelSize: 16
            text: "Score: " + gameEngine.score
                  + "   Lives: " + gameEngine.lives
        }

        // Экран победы/проигрыша
        Rectangle {
            anchors.fill: parent
            visible: gameEngine.gameOver
            color: "#aa000000"

            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 48
                text: gameEngine.levelComplete ? qsTr("Уровень пройден!") : qsTr("Игра окончена")
            }
        }
    }
}