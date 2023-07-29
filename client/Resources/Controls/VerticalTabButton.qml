import QtQuick
import QtQuick.Controls

TabButton {
    id: control
    width: parent.width
    padding: 0

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.checked ? "#ffffff" : "#000000"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        renderType: Text.NativeRendering
        leftPadding: 8
    }

    background: Rectangle {
        implicitHeight: 30
        color: control.checked ? "#0164AC" : "transparent"
    }
}
