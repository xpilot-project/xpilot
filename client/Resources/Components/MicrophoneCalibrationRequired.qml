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
    height: 240
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    focus: true
    closePolicy: Popup.NoAutoClose
    background: Rectangle {
        color: "white"
        border.color: "black"
    }

    Text {
        id: popupText
        text: "<strong>Microphone Calibration Required</strong><br/><br/>Please make sure you have calibrated your microphone volume in the xPilot Settings. Click Settings and verify the microphone level indicator stays green when you speak normally. Use the Mic Volume slider to adjust the microphone volume if necessary.<br/><br/>After you've calibrated your microphone (or if you already have), click Connect again."
        width: 500
        wrapMode: Text.Wrap
        font.pixelSize: 14
        renderType: Text.NativeRendering
        x: 10
        y: 10
    }

    BlueButton {
        id: btnOk
        text: "OK"
        width: 50
        height: 30
        font.pixelSize: 14
        anchors.top: popupText.bottom
        anchors.left: popupText.left
        anchors.topMargin: 10
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                popup.close()
            }
        }
    }
}
