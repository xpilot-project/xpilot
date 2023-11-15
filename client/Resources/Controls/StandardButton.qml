import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Button {
    id: control

    implicitWidth: Math.max(implicitContentWidth + 20, 80)
    implicitHeight: 24

    signal clicked()

    contentItem: Text {
        color: control.enabled ? "#000000" : "#bababa"
        text: control.text
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: control.clicked()
    }

    background: Rectangle {
        border.color: control.enabled ? (mouseArea.containsMouse ? "#006bbe" : "#bababa") : "#dcdcdc"
        color: control.enabled ? (mouseArea.pressed ? "#cce4f7" : mouseArea.containsMouse ? "#e0eef9" : "#ffffff") : "#f9f9f9"
        border.width: 1
        radius: 4
    }
}

