import QtQuick 2.15

Rectangle {
    id: root
    property string text: ""
    signal clicked()
    width: 320
    height: 42
    radius: 7
    color: mouse.containsMouse ? "#0369a1" : "#334155"
    Text {
        anchors { fill: parent; margins: 8 }
        text: root.text
        color: "#f8fafc"
        font.pixelSize: 15
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
