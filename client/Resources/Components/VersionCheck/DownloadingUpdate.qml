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
    width: 570
    height: 180
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose

    property var pctProgress: 0

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
        anchors.topMargin: 10
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
        anchors.topMargin: 10
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
