import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Button {
    id: btn
    height: 35
    contentItem: Text {
        color: '#ffffff'
        text: btn.text
        font.pixelSize: btn.font.pixelSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }
    MouseArea {
        id: btnMouseArea
        hoverEnabled: true
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }
    background: Rectangle {
        color: btnMouseArea.pressed ? '#363b3f' : btnMouseArea.containsMouse ? '#565e64' : btn.enabled ? '#6c757d' : '#c1c1c1'
    }
}
