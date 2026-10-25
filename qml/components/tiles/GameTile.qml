import QtQuick 2.15

Item {
    id: root
    property int cellSize: 32
    property string theme: "classic"
    property bool highlighted: false
    property bool animated: true
    property int animationInterval: 140
    property string tileType: ""
    readonly property TileTheme palette: theme === "kenney" ? kenney
                                        : theme === "industrial" ? industrial : classic
    readonly property url spriteSource: tileType === "brick" ? palette.brickSource
                                       : tileType === "crystal" ? palette.crystalSource
                                       : tileType === "gold" ? palette.prizeSource : ""
    readonly property bool usesSprite: spriteSource.toString().length > 0
    readonly property int imageStatus: sprite.status
    property ClassicTheme classicTheme: ClassicTheme { id: classic }
    property KenneyTheme kenneyTheme: KenneyTheme { id: kenney }
    property IndustrialTheme industrialTheme: IndustrialTheme { id: industrial }
    width: cellSize
    height: cellSize

    Image {
        id: sprite
        anchors.fill: parent
        source: root.spriteSource
        visible: root.usesSprite
        fillMode: Image.PreserveAspectFit
        smooth: false
        mipmap: false
        z: 1
        SequentialAnimation on opacity {
            running: root.animated && root.visible && root.usesSprite && root.tileType === "gold"
            loops: Animation.Infinite
            NumberAnimation { from: 1; to: 0.7; duration: Math.round(600 * root.animationInterval / 140) }
            NumberAnimation { from: 0.7; to: 1; duration: Math.round(600 * root.animationInterval / 140) }
            onRunningChanged: if (!running) sprite.opacity = 1
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.highlighted
        color: "transparent"
        border.color: "#a3e635"
        border.width: 2
        z: 2
    }
}
