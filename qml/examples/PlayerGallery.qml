import QtQuick 2.15
import "../components" as Components

Item {
    id: root
    property int cellSize: 32
    property bool animated: false
    property string previewState: "idle"
    property bool facingRight: true
    height: content.height
    Components.PlayerStyles { id: styles }

    Column {
        id: content
        width: parent.width
        spacing: 12
        Text { text: qsTr("Варианты Персонажа"); color: "#f8fafc"; font.pixelSize: 22; font.bold: true }
        Flow {
            width: parent.width
            spacing: 8
            Repeater {
                model: ["idle", "run", "climb", "dig", "fall"]
                delegate: Rectangle {
                    width: 72; height: 32; radius: 5
                    color: root.previewState === modelData ? "#0369a1" : "#334155"
                    Text { anchors.centerIn: parent; text: modelData; color: "white" }
                    MouseArea { anchors.fill: parent; onClicked: root.previewState = modelData }
                }
            }
            Rectangle {
                width: 128; height: 32; radius: 5; color: "#334155"
                Text { anchors.centerIn: parent; text: root.facingRight ? qsTr("Вправо →") : qsTr("← Влево"); color: "white" }
                MouseArea { anchors.fill: parent; onClicked: root.facingRight = !root.facingRight }
            }
        }
        Flow {
            width: parent.width
            spacing: 12
            Repeater {
                model: styles.variants
                delegate: Rectangle {
                    width: Math.min(208, content.width); height: 144; radius: 8; color: "#1e293b"
                    Components.Player {
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 12 + (76 - size) / 2
                        size: root.cellSize
                        variant: modelData.key
                        state: root.previewState
                        facingRight: root.facingRight
                        animated: root.animated
                    }
                    Text {
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 10 }
                        text: modelData.label
                        color: "#e2e8f0"; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
