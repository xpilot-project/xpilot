import QtQuick 2.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import AppConfig 1.0
import "../../Controls"

Popup {
    id: popup
    width: 550
    height: 120
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose

    Text {
        id: labelAskDownload
        text: "A new version of xPilot is available. Would you like to download and install it now?"
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
        anchors.top: labelAskDownload.bottom
        anchors.left: labelAskDownload.left
        anchors.topMargin: 10
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                versionCheck.downloadInstaller()
                popup.close()
            }
        }
    }

    GrayButton {
        id: btnNo
        text: "No"
        width: 50
        height: 30
        font.pixelSize: 14
        anchors.top: labelAskDownload.bottom
        anchors.left: btnYes.right
        anchors.topMargin: 10
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
