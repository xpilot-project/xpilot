import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Window
import QtQuick.Layouts
import AppConfig 1.0
import "../../Controls"

Popup {
    id: popup
    width: 650
    height: 200
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    background: Rectangle {
        color: "white"
        border.color: "black"
    }

    signal closeWindow()

    property double pctProgress: 0

    Component.onCompleted: {
        labelDownloading.visible = false
        progressBar.visible = false
        labelPercent.visible = false
        btnCancelDownload.visible = false
        labelAskDownload.visible = true
        tokenValidationError.visible = false
    }

    Connections {
        target: installModels

        function onTokenValidationError(errorText) {
            tokenValidationError.text = errorText
            tokenValidationError.visible = true
        }

        function onDownloadStarted() {
            labelDownloading.visible = true
            progressBar.visible = true
            labelPercent.visible = true
            btnCancelDownload.visible = true
            labelAskDownload.visible = false
            tokenValidationError.visible = false
        }
    }

    // Ask

    Text {
        id: labelAskDownload
        text: "It looks like this is your first time using xPilot. Before you can connect to the network, you must " +
              "install a CSL aircraft model set. Would you like to download and install one now?\r\n\r\nDownload size: Approximately 560MB.\r\n\r\n" +
              "If you choose No, you will have to manually install a model set yourself, or use an existing model set that you already have installed.\r\n"
        font.pixelSize: 14
        renderType: Text.NativeRendering
        width: 600
        wrapMode: Text.Wrap
        x: 15
        y: 10
    }

    BlueButton {
        id: btnYes
        text: "Yes"
        width: 50
        height: 30
        font.pixelSize: 14
        anchors.top: labelAskDownload.bottom
        anchors.left: labelAskDownload.left
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                installModels.downloadModels()
                btnNo.visible = false
                btnYes.visible = false
                labelAskDownload.visible = false
                chkDontAskAgainModels.visible = false
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
        anchors.leftMargin: 10
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if(chkDontAskAgainModels.checked) {
                    AppConfig.SilenceModelInstall = true
                    AppConfig.saveConfig()
                }
                closeWindow()
            }
        }
    }

    CheckBox {
        id: chkDontAskAgainModels
        text: qsTr("Don't ask me again")
        anchors.top: btnNo.top
        anchors.left: btnNo.right
        anchors.leftMargin: 10
        font.pixelSize: 14

        indicator: Rectangle {
            implicitWidth: 16
            implicitHeight: 16
            x: chkDontAskAgainModels.leftPadding
            y: parent.height / 2 - height / 2
            radius: 3
            border.color: chkDontAskAgainModels.down ? "#272C2E" : "#565E64"

            Rectangle {
                width: 8
                height: 8
                x: 4
                y: 4
                radius: 2
                color: chkDontAskAgainModels.down ? "#272C2E" : "#565E64"
                visible: chkDontAskAgainModels.checked
            }
        }

        contentItem: Text {
            text: chkDontAskAgainModels.text
            font: chkDontAskAgainModels.font
            opacity: enabled ? 1.0 : 0.3
            color: chkDontAskAgainModels.down ? "#272C2E" : "#565E64"
            verticalAlignment: Text.AlignVCenter
            leftPadding: chkDontAskAgainModels.indicator.width + chkDontAskAgainModels.spacing
            renderType: Text.NativeRendering
        }
    }

    // Downloading

    Label {
        id: tokenValidationError
        visible: false
        font.pixelSize: 14
        color: '#D50005'
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        anchors.top: chkDontAskAgainModels.bottom
        x: 15
    }

    Label {
        id: labelDownloading
        visible: false
        text: "Downloading CSL aircraft model set. Please wait..."
        font.pixelSize: 14
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        x: 20
        y: 40
    }

    ProgressBar {
        id: progressBar
        visible: false
        value: popup.pctProgress
        anchors.top: labelDownloading.bottom
        anchors.left: labelDownloading.left
        anchors.topMargin: 10
        padding: 5

        background: Rectangle {
            implicitWidth: 530
            implicitHeight: 6
            color: "#e6e6e6"
            radius: 3
        }

        contentItem: Item {
            implicitWidth: 530
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
        visible: false
        font.pixelSize: 14
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        anchors.top: progressBar.top
        anchors.left: progressBar.right
        anchors.leftMargin: 10
    }

    GrayButton {
        id: btnCancelDownload
        visible: false
        text: "Cancel Download"
        width: 120
        height: 30
        font.pixelSize: 14
        anchors.left: progressBar.left
        anchors.top: progressBar.bottom
        anchors.topMargin: 15
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                installModels.cancel()
                closeWindow()
            }
        }
    }
}
