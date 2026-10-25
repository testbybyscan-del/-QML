import QtQuick 2.15

Item {
    id: player
    width: size
    height: size

    property int size: 32
    property string state: "idle" // idle, run, climb, dig, fall — состояние из GameEngine
    property bool facingRight: true
    property string variant: "classic"
    property bool animated: true
    property int animationInterval: 140
    property int animationFrame: 0
    readonly property var selectedStyle: styles.style(variant)
    readonly property string variantLabel: selectedStyle.label
    readonly property bool usesSprite: selectedStyle.frames.length > 0
    readonly property int imageStatus: sprite.status
    readonly property bool moving: state === "run" || state === "climb" || state === "dig"
    readonly property url spriteSource: {
        if (!usesSprite) return ""
        var index = animated && moving ? animationFrame % selectedStyle.frames.length : 0
        var number = ("0000" + selectedStyle.frames[index]).slice(-4)
        return "qrc:/assets/tilesets/kenney/pixel-platformer/Tiles/Characters/tile_" + number + ".png"
    }

    PlayerStyles { id: styles }
    onStateChanged: animationFrame = 0
    onVariantChanged: animationFrame = 0
    onAnimatedChanged: animationFrame = 0

    Timer {
        interval: player.animationInterval
        repeat: true
        running: player.visible && player.animated && player.usesSprite && player.moving
        onTriggered: player.animationFrame = (player.animationFrame + 1) % player.selectedStyle.frames.length
    }

    Image {
        id: sprite
        objectName: "playerSprite"
        anchors.fill: parent
        source: player.spriteSource
        visible: player.usesSprite
        fillMode: Image.PreserveAspectFit
        smooth: false
        mipmap: false
        mirror: !player.facingRight
        // Наклон — визуальное обозначение действия, а не изменение координат движка.
        rotation: player.state === "dig" ? (player.facingRight ? 12 : -12)
                  : player.state === "fall" ? (player.facingRight ? 6 : -6) : 0
    }

    Item {
        anchors.fill: parent
        visible: !player.usesSprite
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.7
            height: parent.height * 0.7
            radius: 4
            color: player.state === "dig" ? "#ffa500" : "#32cd32"
            border.color: "black"
            border.width: 1
            Rectangle {
                width: player.size / 8; height: width
                radius: width / 2
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                x: player.facingRight ? parent.width - player.size / 4 : player.size / 8
            }
        }
    }
}
