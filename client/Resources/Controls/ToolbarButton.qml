import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Button {
    id: button
    x: 20
    checked: false
    checkable: false
    text: qsTr("BUTTON")
    font.pixelSize: 13
    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: 0
    width: text.contentWidth
    height: 30

    property bool active: false

    MouseArea {
        id: btnMouseArea
        hoverEnabled: true
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    background: Rectangle {
        color: active ? "#0164AD" : (btnMouseArea.pressed ? "#a7acb1" : button.enabled && btnMouseArea.containsMouse ? "#565e64" : "transparent")
        opacity: button.enabled ? 1 : 0.5
        border.color: active ? '#0078CE' : '#6c757d'
    }

    contentItem: Text {
        color: (active || enabled) ? "#ffffff" : "#6c757d"
        text: button.text.toUpperCase()
        font.pixelSize: button.font.pixelSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.FixedSize
        opacity: button.enabled ? 1 : 0.5
        font.styleName: Font.Normal
        font.family: robotoMono.name
        renderType: Text.NativeRendering
    }
}
