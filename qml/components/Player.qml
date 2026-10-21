import QtQuick 2.15

Item {
    id: player
    width: size
    height: size

    // Внешние свойства
    property int size: 32
    property string state: "idle"       // "idle", "run", "climb", "dig", "fall"
    property bool facingRight: true

    // Запасная отрисовка простыми фигурами (пока нет спрайтов)
    // Если у вас есть спрайт-лист, замените этот Item на SpriteSequence.
    Item {
        anchors.fill: parent

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.7
            height: parent.height * 0.7
            radius: 4
            color: player.state === "dig" ? "#ffa500" : "#32cd32"
            border.color: "black"
            border.width: 1

            // Условный "глаз" для указания направления
            Rectangle {
                width: 4; height: 4
                radius: 2
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                x: player.facingRight ? parent.width - 8 : 4
            }
        }
    }

    // Пример SpriteSequence — раскомментируйте, когда будут спрайты.
    /*
    SpriteSequence {
        anchors.fill: parent
        width: player.size
        height: player.size

        Sprite {
            name: "idle"
            source: "qrc:/assets/images/player_idle.png"
            frameCount: 1
            frameRate: 1
        }
        Sprite {
            name: "run"
            source: "qrc:/assets/images/player_run.png"
            frameCount: 4
            frameRate: 10
        }
        Sprite {
            name: "climb"
            source: "qrc:/assets/images/player_climb.png"
            frameCount: 2
            frameRate: 5
        }
        Sprite {
            name: "dig"
            source: "qrc:/assets/images/player_dig.png"
            frameCount: 3
            frameRate: 8
        }
        Sprite {
            name: "fall"
            source: "qrc:/assets/images/player_fall.png"
            frameCount: 2
            frameRate: 8
        }
    }
    */

    // Реагируем на смену состояния — переключаем анимацию
    onStateChanged: {
        // playerSprite.jumpTo(state)
    }
}
