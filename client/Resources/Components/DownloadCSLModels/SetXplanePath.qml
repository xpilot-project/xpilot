import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs
import AppConfig 1.0
import "../../Controls"

Popup {
        id: popup
        width: 600
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

        Connections {
            target: installModels

            function onInvalidXplanePath(errorText) {
                xplanePathUrl.color = "red"
                xplanePathUrl.text = errorText
            }
        }

        Label {
            id: lblXplanePath
            text: "Please browse to the folder where X-Plane is installed:"
            font.pixelSize: 14
            renderType: Text.NativeRendering
            wrapMode: Text.WordWrap
            x: 20
            y: 30
        }

        BlueButton {
            text: "Select Folder..."
            width: 120
            height: 30
            font.pixelSize: 14
            anchors.left: lblXplanePath.right
            anchors.leftMargin: 10
            y: 25
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    xplanePath.open()
                }
            }
        }

        FolderDialog {
            id: xplanePath
            title: "Select X-Plane Folder"
            onAccepted: {
                var path = xplanePath.selectedFolder.toString()
                path = path.replace(/^file:\/{3}/,"")
                xplanePathUrl.color = "black"
                xplanePathUrl.text = `<b>Path:</b> ${path}`
            }
        }

        Text {
            id: xplanePathUrl
            anchors.top: lblXplanePath.bottom
            anchors.left: lblXplanePath.left
            anchors.topMargin: 20
            font.pixelSize: 14
            renderType: Text.NativeRendering
            wrapMode: Text.WordWrap
            width: 480
        }

        BlueButton {
            id: xplanePathBtnOk
            text: "OK"
            width: 80
            height: 30
            font.pixelSize: 14
            anchors.topMargin: 25
            anchors.top: xplanePathUrl.bottom
            anchors.left: xplanePathUrl.left
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    installModels.validatePath(xplanePath.selectedFolder.toString())
                }
            }
        }

        GrayButton {
            id: xplanePathBtnCancel
            text: "Cancel"
            width: 80
            height: 30
            font.pixelSize: 14
            anchors.topMargin: 25
            anchors.top: xplanePathUrl.bottom
            anchors.left: xplanePathBtnOk.right
            anchors.leftMargin: 10
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
