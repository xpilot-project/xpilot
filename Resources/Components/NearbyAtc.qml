import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Rectangle {
    id: nearbyAtc
    Layout.preferredWidth: 250
    Layout.fillHeight: true
    Layout.row: 1
    Layout.column: 0
    color: "#272c2e"
    Layout.alignment: Qt.AlignHCenter | Qt.AlignTop

    Text {
        id: title
        color: "white"
        text: qsTr("Nearby ATC")
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        font.pixelSize: 14
        font.bold: true
        font.family: robotoMono.name
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        padding: 5
    }
}
