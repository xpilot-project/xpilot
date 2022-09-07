import QtQuick 2.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
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

        FileDialog {
            id: xplanePath
            title: "Select X-Plane Folder"
            selectFolder: true
            onAccepted: {
                var path = xplanePath.folder.toString()
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
                    installModels.validatePath(xplanePath.folder.toString())
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
