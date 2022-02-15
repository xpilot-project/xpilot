import QtQuick 2.0
import QtQuick.Controls 2.12

Button {
    id: btn
    height: 35
    contentItem: Text {
        color: '#ffffff'
        text: btn.text
        font.pointSize: btn.font.pointSize
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
