import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Window
import QtQuick.Layouts
import AppConfig 1.0
import "../Controls"

Popup {
    id: popup
    width: 550
    height: 125
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    background: Rectangle {
        color: "white"
        border.color: "black"
    }

    signal exitApplication()

    Text {
        id: lblAskDisconnect
        text: "You are still connected to the network. Are you sure you want to close xPilot?"
        width: 500
        wrapMode: Text.Wrap
        font.pixelSize: 14
        renderType: Text.NativeRendering
        x: 20
        y: 20
    }

    BlueButton {
        id: btnYes
        text: "Yes"
        width: 50
        height: 30
        font.pixelSize: 14
        anchors.top: lblAskDisconnect.bottom
        anchors.left: lblAskDisconnect.left
        anchors.topMargin: 15
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                exitApplication()
            }
        }
    }

    GrayButton {
        id: btnNo
        text: "No"
        width: 50
        height: 30
        font.pixelSize: 14
        anchors.top: lblAskDisconnect.bottom
        anchors.left: btnYes.right
        anchors.topMargin: 15
        anchors.leftMargin: 10
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                popup.close()
            }
        }
    }
}
