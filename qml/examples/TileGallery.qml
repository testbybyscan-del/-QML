import QtQuick 2.15
import QtQuick.Window 2.15
import "../components/tiles" as Tiles

Window {
    id: gallery
    visible: true
    width: 960
    height: 680
    minimumWidth: 360
    minimumHeight: 440
    title: qsTr("Галерея игровых тайлов")
    color: "#111827"
    property int previewSize: 32
    property bool animateTiles: false
    property bool highlightTiles: false
    readonly property var themes: ["classic", "kenney", "industrial"]

    Rectangle { anchors.fill: parent; color: "#111827"; z: -1 }

    Column {
        id: header
        x: 24; y: 20
        width: parent.width - 48
        spacing: 10
        Text {
            text: qsTr("Библиотека тайлов")
            color: "#f8fafc"; font.pixelSize: 26; font.bold: true
        }
        Text {
            width: parent.width
            text: qsTr("Три оформления · Кирпич, Кристалл и Приз · Без игрового движка")
            color: "#94a3b8"; font.pixelSize: 14; wrapMode: Text.WordWrap
        }
        Flow {
            width: parent.width
            spacing: 8
            Repeater {
                model: [18, 32, 48, 64]
                delegate: Rectangle {
                    width: 64; height: 34; radius: 6
                    color: gallery.previewSize === modelData ? "#0369a1" : "#334155"
                    Text { anchors.centerIn: parent; text: modelData + " px"; color: "white" }
                    MouseArea { anchors.fill: parent; onClicked: gallery.previewSize = modelData }
                }
            }
            Rectangle {
                width: 140; height: 34; radius: 6
                color: gallery.animateTiles ? "#0369a1" : "#334155"
                Text { anchors.centerIn: parent; text: qsTr("Анимация: %1").arg(gallery.animateTiles ? "вкл" : "выкл"); color: "white" }
                MouseArea { anchors.fill: parent; onClicked: gallery.animateTiles = !gallery.animateTiles }
            }
            Rectangle {
                width: 140; height: 34; radius: 6
                color: gallery.highlightTiles ? "#0369a1" : "#334155"
                Text { anchors.centerIn: parent; text: qsTr("Подсветка: %1").arg(gallery.highlightTiles ? "вкл" : "выкл"); color: "white" }
                MouseArea { anchors.fill: parent; onClicked: gallery.highlightTiles = !gallery.highlightTiles }
            }
        }
    }

    Flickable {
        anchors { top: header.bottom; topMargin: 20; bottom: parent.bottom; left: parent.left; right: parent.right; margins: 24 }
        clip: true
        contentWidth: width
        contentHeight: cards.height
        Flow {
            id: cards
            width: parent.width
            spacing: 14
            Repeater {
                model: gallery.themes
                delegate: Rectangle {
                    id: card
                    readonly property string themeName: modelData
                    width: Math.min(280, cards.width)
                    height: 402
                    radius: 10; color: "#1e293b"
                    border.color: "#334155"
                    Column {
                        anchors { fill: parent; margins: 16 }
                        spacing: 14
                        Text { text: card.themeName.toUpperCase(); color: "#e2e8f0"; font.bold: true; font.pixelSize: 18 }
                        Repeater {
                            model: ["brick", "crystal", "gold"]
                            delegate: Item {
                                id: sample
                                property string kind: modelData
                                width: card.width - 32
                                height: 96
                                Rectangle {
                                    width: 76; height: 76; color: "#172033"
                                    // Two backgrounds expose sprite transparency.
                                    Rectangle { x: 38; width: 38; height: 76; color: "#475569" }
                                    Loader {
                                        anchors.centerIn: parent
                                        sourceComponent: sample.kind === "brick" ? brick : sample.kind === "crystal" ? crystal : prize
                                    }
                                }
                                Text {
                                    x: 88; width: parent.width - x; y: 8
                                    text: sample.kind === "brick" ? qsTr("Кирпич") : sample.kind === "crystal" ? qsTr("Кристалл") : qsTr("Приз")
                                    color: "#f1f5f9"; font.pixelSize: 16
                                }
                                Text {
                                    x: 88; width: parent.width - x; y: 34
                                    text: {
                                        if (card.themeName === "classic") return qsTr("Геометрия QML")
                                        var urls = card.themeName === "kenney" ? kenney : industrial
                                        var url = sample.kind === "brick" ? urls.brickSource : sample.kind === "crystal" ? urls.crystalSource : urls.prizeSource
                                        return url.toString().split("/").pop() + "\n18 × 18 · CC0"
                                    }
                                    color: "#94a3b8"; font.pixelSize: 12; wrapMode: Text.WrapAnywhere
                                }
                                Component { id: brick; Tiles.BrickTile { cellSize: gallery.previewSize; theme: card.themeName; animated: gallery.animateTiles; highlighted: gallery.highlightTiles } }
                                Component { id: crystal; Tiles.CrystalTile { cellSize: gallery.previewSize; theme: card.themeName; animated: gallery.animateTiles; highlighted: gallery.highlightTiles } }
                                Component { id: prize; Tiles.PrizeTile { cellSize: gallery.previewSize; theme: card.themeName; animated: gallery.animateTiles; highlighted: gallery.highlightTiles } }
                            }
                        }
                    }
                }
            }
            Rectangle {
                width: Math.min(868, cards.width); height: 108; radius: 10; color: "#1e293b"
                Image {
                    x: 16; anchors.verticalCenter: parent.verticalCenter
                    width: gallery.previewSize; height: width
                    source: "qrc:/assets/tilesets/kenney/pixel-platformer-blocks/Tiles/Stone/tile_0000.png"
                    fillMode: Image.PreserveAspectFit; smooth: false
                }
                Text {
                    x: 96; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 112
                    text: qsTr("Дополнительный вариант: Blocks / Stone / tile_0000.png\n18 × 18 · CC0 · кандидат для будущей темы")
                    wrapMode: Text.WrapAnywhere; color: "#cbd5e1"
                }
            }
            PlayerGallery {
                width: cards.width
                cellSize: gallery.previewSize
                animated: gallery.animateTiles
            }
        }
    }
    Tiles.KenneyTheme { id: kenney }
    Tiles.IndustrialTheme { id: industrial }
}
