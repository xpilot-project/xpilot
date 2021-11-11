import QtQuick 2.0
import QtQuick.Controls 2.12

Button {
    id: btn
    height: 35
    contentItem: Text {
        color: '#ffffff'
        text: btn.text
        font.family: ubuntuRegular.name
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
        color: btnMouseArea.pressed ? '#363b3f' : btnMouseArea.containsMouse ? '#565e64' : '#6c757d'
    }
}
