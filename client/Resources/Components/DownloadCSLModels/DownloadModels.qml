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
    height: 220
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
            labelToken.visible = false
            labelDownloadToken.visible = false
            labelDownloadTokenId.visible = false
            downloadTokenId.visible = false
            downloadToken.visible = false
            btnBeginDownload.visible = false
            btnGetToken.visible = false
            btnCancelToken.visible = false
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
                installModels.checkIfZipExists()

                btnNo.visible = false
                btnYes.visible = false
                labelAskDownload.visible = false
                chkDontAskAgainModels.visible = false

                btnGetToken.visible = true
                btnCancelToken.visible = true
                labelToken.visible = true
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

    // Verify Identity

    Text {
        id: labelToken
        text: "Before you can install the CSL model package, you must authenticate yourself using your VATSIM credentials. Click the \"Get Token\" button below to generate a download token. Your internet browser will open to a website with your download token."
        visible: false
        font.pixelSize: 14
        renderType: Text.NativeRendering
        width: 600
        wrapMode: Text.Wrap
        x: 15
        y: 10
    }

    Label {
        id: labelDownloadToken
        color: "#333333"
        text: "Token"
        visible: false
        renderType: Text.NativeRendering
        font.pixelSize: 13
        anchors.top: labelToken.bottom
        anchors.topMargin: 15
        x: 15
    }

    Item {
        id: downloadToken
        visible: false
        anchors.top: labelDownloadToken.bottom
        anchors.topMargin: 5
        x: 15
        width: 100

        CustomTextField {
            id: txtDownloadToken
            maximumLength: 6
            validator: RegularExpressionValidator {
                regularExpression: /[0-9]+/
            }
        }
    }

    Label {
        id: labelDownloadTokenId
        color: "#333333"
        text: "VATSIM ID"
        visible: false
        renderType: Text.NativeRendering
        font.pixelSize: 13
        anchors.top: labelToken.bottom
        anchors.left: downloadToken.right
        anchors.leftMargin: 10
        anchors.topMargin: 15
        x: 15
    }

    Item {
        id: downloadTokenId
        visible: false
        anchors.left: downloadToken.right
        anchors.top: labelDownloadTokenId.bottom
        anchors.topMargin: 5
        anchors.leftMargin: 10
        x: 15
        width: 100

        CustomTextField {
            id: txtDownloadTokenId
            maximumLength: 8
            validator: RegularExpressionValidator {
                regularExpression: /[0-9]+/
            }
        }
    }

    BlueButton {
        id: btnGetToken
        visible: false
        text: "Get Token"
        width: 130
        height: 30
        font.pixelSize: 14
        anchors.topMargin: 40
        anchors.left: labelToken.left
        anchors.top: labelToken.bottom
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                Qt.openUrlExternally("https://xpilot-project.org/CdnAuth")

                btnGetToken.visible = false
                btnBeginDownload.visible = true

                labelDownloadToken.visible = true
                downloadToken.visible = true

                if(!AppConfig.VatsimId) {
                    downloadTokenId.visible = true
                    labelDownloadTokenId.visible = true
                }
            }
        }
    }

    BlueButton {
        id: btnBeginDownload
        visible: false
        text: "Begin Download"
        width: 130
        height: 30
        font.pixelSize: 14
        anchors.leftMargin: 20
        anchors.left: downloadTokenId.visible ? downloadTokenId.right : downloadToken.right
        anchors.top: downloadTokenId.visible ? downloadTokenId.top : downloadToken.top
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                installModels.downloadModels(txtDownloadToken.text, AppConfig.VatsimId || txtDownloadTokenId.text)
            }
        }
    }

    GrayButton {
        id: btnCancelToken
        visible: false
        text: "Cancel"
        width: 80
        height: 30
        font.pixelSize: 14
        anchors.leftMargin: 15
        anchors.left: btnBeginDownload.visible ? btnBeginDownload.right : btnGetToken.right
        anchors.top: downloadToken.visible ? downloadToken.top : btnGetToken.top
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                installModels.cancel()
                closeWindow()
            }
        }
    }

    Label {
        id: tokenValidationError
        visible: false
        font.pixelSize: 14
        color: '#D50005'
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        anchors.top: downloadToken.bottom
        anchors.topMargin: 50
        x: 15
    }


    // Downloading

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
