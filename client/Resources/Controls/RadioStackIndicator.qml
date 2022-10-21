import QtQuick

Rectangle {
    property bool isEnabled: false
    property bool isActive: false
    property string label: "XX"

    id: indicator
    width: 25
    height: 20
    color: isEnabled ? (isActive ? '#27ae60' : '#015a9b') : '#0162A9'
    anchors.verticalCenter: parent.verticalCenter
    border {
        color: isEnabled ? '#00080e' : '#01528D'
    }
    Text {
        id: txLabel
        text: label
        anchors.fill: parent
        font.pixelSize: 10
        font.family: robotoMono.name
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: isEnabled ? 1 : 0.2
        color: "#ffffff"
        renderType: Text.NativeRendering
    }
}
