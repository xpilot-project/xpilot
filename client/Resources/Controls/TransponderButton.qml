import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Button {
    property bool isActive: false

    id: button
    text: qsTr("Button")
    checked: false
    checkable: false
    font.pixelSize: 13
    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: 0
    width: text.contentWidth
    height: 30
    x: 20

    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    background: Rectangle {
        color: isActive ? (mouseArea.pressed ? "#145423" : button.enabled && mouseArea.containsMouse ? "#208637" : "#28a745") : button.enabled && mouseArea.containsMouse ? "#565e64" : "transparent"
        opacity: button.enabled ? 1 : 0.5
        border.color: isActive ? (mouseArea.pressed ? "#145423" : "#28a745") : "#6c757d"
    }

    contentItem: Text {
        id: label
        color: button.enabled || (button.enabled && isActive) || (button.enabled && mouseArea.pressed) || (button.enabled && mouseArea.containsMouse) ? "#ffffff" : "#6c757d"
        text: button.text.toUpperCase()
        font.pixelSize: button.font.pixelSize
        font.family: robotoMono.name
        font.styleName: Font.Normal
        opacity: button.enabled ? 1 : 0.5
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }
}
