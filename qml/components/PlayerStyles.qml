import QtQuick 2.15

QtObject {
    readonly property var variants: [
        { key: "classic", label: qsTr("Классический"), frames: [] },
        { key: "astronaut", label: qsTr("Космонавт"), frames: [0, 1] },
        { key: "pink", label: qsTr("Розовый космонавт"), frames: [4, 5] },
        { key: "robot", label: qsTr("Робот"), frames: [18, 19, 20] }
    ]
    function style(key) {
        for (var i = 0; i < variants.length; ++i)
            if (variants[i].key === key) return variants[i]
        return variants[0]
    }
    function next(key) {
        for (var i = 0; i < variants.length; ++i)
            if (variants[i].key === key) return variants[(i + 1) % variants.length].key
        return variants[0].key
    }
}
