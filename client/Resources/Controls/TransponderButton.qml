import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    property bool isActive: false

    id: button
    text: qsTr("Button")
    checked: false
    checkable: false
    font.pixelSize: 13
    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: 0
    width: metrics.advanceWidth + 20
    height: 30
    x: 20

    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
    }

    background: Rectangle {
        color: isActive ? (mouseArea.pressed ? "#145423" : mouseArea.containsMouse ? "#208637" : "#28a745") : mouseArea.containsMouse ? "#565e64" : "transparent"
        opacity: button.enabled ? 1 : 0.5
        border.color: isActive ? (mouseArea.pressed ? "#145423" : "#28a745") : "#6c757d"
    }

    contentItem: Text {
        id: label
        color: (isActive || enabled || mouseArea.pressed || mouseArea.containsMouse) ? "#ffffff" : "#6c757d"
        text: button.text.toUpperCase()
        font.pixelSize: button.font.pixelSize
        font.family: robotoMono.name
        font.styleName: Font.Normal
        opacity: button.enabled ? 1 : 0.5
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    TextMetrics {
        id: metrics
        font: label.font
        text: label.text
    }
}
