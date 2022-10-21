import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Window
import QtQuick.Layouts
import AppConfig 1.0
import "../../Controls"

Popup {
    id: popup
    width: 570
    height: 210
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    background: Rectangle {
        color: "white"
        border.color: "black"
    }

    property real pctProgress: 0

    Connections {
        target: versionCheck

        function onDownloadPercentChanged(pct) {
            pctProgress = pct
        }

        function onDownloadFinished() {
            btnCancel.enabled = false
        }
    }

    Text {
        id: promptText
        text: "Downloading xPilot update. Please wait..."
        width: 500
        wrapMode: Text.Wrap
        font.pixelSize: 14
        renderType: Text.NativeRendering
        x: 20
        y: 20
    }

    ProgressBar {
        id: progressBar
        value: popup.pctProgress
        anchors.top: promptText.bottom
        anchors.left: promptText.left
        anchors.topMargin: 15
        padding: 5

        background: Rectangle {
            implicitWidth: 430
            implicitHeight: 6
            color: "#e6e6e6"
            radius: 3
        }

        contentItem: Item {
            implicitWidth: 430
            implicitHeight: 6

            Rectangle {
                width: progressBar.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: "#0164AD"
            }
        }
    }

    Label {
        id: labelPercent
        text: (popup.pctProgress * 100).toFixed(0) + "%"
        font.pixelSize: 14
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        anchors.top: progressBar.top
        anchors.left: progressBar.right
        anchors.leftMargin: 10
    }

    Text {
        id: prompt2
        text: "xPilot will automatically close and the installer will launch after the download finishes."
        width: 500
        wrapMode: Text.Wrap
        font.pixelSize: 14
        anchors.top: progressBar.bottom
        anchors.left: progressBar.left
        anchors.topMargin: 15
        renderType: Text.NativeRendering
    }

    GrayButton {
        id: btnCancel
        text: "Cancel"
        width: 80
        height: 30
        font.pixelSize: 14
        anchors.top: prompt2.bottom
        anchors.left: prompt2.left
        anchors.topMargin: 15
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                versionCheck.cancelDownload()
                popup.close()
            }
        }
    }
}
