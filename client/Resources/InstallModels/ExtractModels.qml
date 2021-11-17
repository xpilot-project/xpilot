import QtQuick 2.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import AppConfig 1.0
import "../Controls"

Popup {
    id: popup
    width: 600
    height: 180
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose

    property double pctProgress: 0

    Label {
        id: lblUnzipping
        text: "Installing CSL aircraft model set. Please wait..."
        font.pointSize: 10
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        x: 20
        y: 40
    }

    ProgressBar {
        id: progressBar
        value: popup.pctProgress
        anchors.top: lblUnzipping.bottom
        anchors.left: lblUnzipping.left
        anchors.topMargin: 10
        padding: 5

        background: Rectangle {
            implicitWidth: 480
            implicitHeight: 6
            color: "#e6e6e6"
            radius: 3
        }

        contentItem: Item {
            implicitWidth: 480
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
        id: lblPercent
        text: (popup.pctProgress * 100).toFixed(0) + "%"
        font.pointSize: 10
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        anchors.top: progressBar.top
        anchors.left: progressBar.right
        anchors.leftMargin: 10
    }

    GrayButton {
        id: btnCancel
        text: "Cancel"
        width: 80
        height: 30
        font.pointSize: 10
        anchors.topMargin: 10
        anchors.top: progressBar.bottom
        anchors.left: progressBar.left
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                installModels.cancel()
                popup.close()
            }
        }
    }
}
