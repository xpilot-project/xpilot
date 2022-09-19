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
        color: btnMouseArea.pressed ? '#013257' : btnMouseArea.containsMouse ? '#01508a' : '#0164ad'
    }
}
