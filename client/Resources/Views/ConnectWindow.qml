import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import AppConfig 1.0
import "../Components"
import "../Controls"

Popup {
    id: popup
    width: 500
    height: 180
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose

    signal closeWindow()

    Component.onCompleted: {
        txtCallsign.text = AppConfig.RecentConnection.Callsign;
        txtTypeCode.text = AppConfig.RecentConnection.TypeCode;
        txtSelcal.text = AppConfig.RecentConnection.SelcalCode;
    }

    Popup {
        id: validationPopup
        width: 400
        height: 120
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose
        margins: 20

        property var errorMessage: ""

        Text {
            id: errorLabel
            text: validationPopup.errorMessage
            font.pixelSize: 14
            renderType: Text.NativeRendering
            width: parent.width
            height: 55
            wrapMode: Text.Wrap
            color: "#C0392B"
            verticalAlignment: Text.AlignVCenter
            bottomPadding: 10
            leftPadding: 10
        }

        GrayButton {
            id: btnOK
            text: "OK"
            width: 80
            height: 30
            font.pixelSize: 14
            anchors.top: errorLabel.bottom 
            x: 10
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    validationPopup.close()
                }
            }
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.rightMargin: 30
        anchors.leftMargin: 30
        anchors.bottomMargin: 15
        anchors.topMargin: 15
        columns: 3
        rows: 3

        Item {
            id: callsign
            Layout.preferredHeight: 35
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 0
            Layout.column: 0
            Layout.row: 1

            Text {
                color: "#000"
                text: qsTr("Callsign")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }


            CustomTextField {
                id: txtCallsign
                height: 28
                anchors.top: callsign.bottom
                anchors.topMargin: -5
                selectByMouse: true
                maximumLength: observerMode.checked ? 8 : 7
                onTextChanged: {
                    text = text.toUpperCase()
                }
            }
        }

        Item {
            id: typeCode
            Layout.preferredHeight: 35
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 0
            Layout.column: 1
            Layout.row: 1

            Text {
                color: "#000"
                text: qsTr("Type Code")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }


            CustomTextField {
                id: txtTypeCode
                height: 28
                anchors.top: typeCode.bottom
                anchors.topMargin: -5
                selectByMouse: true
                onTextChanged: {
                    text = text.toUpperCase()
                }
            }
        }

        Item {
            id: selcal
            Layout.preferredHeight: 35
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 0
            Layout.column: 2
            Layout.row: 1

            Text {
                color: "#000"
                text: qsTr("SELCAL")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }


            CustomTextField {
                id: txtSelcal
                height: 28
                anchors.top: selcal.bottom
                anchors.topMargin: -5
                selectByMouse: true
                maximumLength: 5
                onTextChanged: {
                    text = text.toUpperCase()
                }
            }
        }

        Item {
            id: connectObserver
            Layout.preferredHeight: 40
            Layout.fillHeight: true
            Layout.column: 0
            Layout.row: 2
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.topMargin: 20

            CustomCheckBox {
                id: observerMode
                text: "Connect in Shared Cockpit/Observer Mode"
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                onCheckedChanged: {
                    txtCallsign.text = observerMode.checked ? txtCallsign.text.substring(0,8) : txtCallsign.text.substring(0,7);
                }
            }
        }

        Item {
            id: connectButton
            Layout.preferredHeight: 40
            Layout.fillHeight: true
            Layout.columnSpan: 2
            Layout.row: 3
            Layout.column: 0
            Layout.fillWidth: true

            BlueButton {
                id: blueButton
                text: qsTr("Connect to VATSIM")
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottomMargin: 0
                anchors.topMargin: 0
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                font.pixelSize: 13
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if(txtSelcal.text !== "") {
                            const prohibitedChars = ["I", "N", "O"]
                            var selcal = txtSelcal.text
                            selcal = selcal.replace("-", "")
                            selcal = selcal.toUpperCase()
                            const chars = selcal.split("")
                            if (/([a-zA-Z]).*?\1/.test(selcal)) {
                                validationPopup.errorMessage = "SELCAL cannot contain repeating characters."
                                validationPopup.open()
                                return
                            }
                            for (let i = 1; i < chars.length; i += 2) {
                                if (chars[i] > 'S') {
                                    validationPopup.errorMessage = "SELCAL can only contain characters A through S."
                                    validationPopup.open()
                                    return
                                }
                                if (chars[i - 1] > 'R') {
                                    validationPopup.errorMessage = "SELCAL can only contain characters A through S."
                                    validationPopup.open()
                                    return
                                }
                                if (prohibitedChars.includes(chars[i])) {
                                    validationPopup.errorMessage = "SELCAL cannot contain characters I, N or O."
                                    validationPopup.open()
                                    return
                                }
                                if (chars[i] < chars[i - 1]) {
                                    validationPopup.errorMessage = "SELCAL characters must be in ascending, alphabetical order."
                                    validationPopup.open()
                                    return
                                }
                            }
                        }

                        networkManager.connectToNetwork(txtCallsign.text, txtTypeCode.text, txtSelcal.text, observerMode.checked);
                        AppConfig.RecentConnection.Callsign = txtCallsign.text;
                        AppConfig.RecentConnection.TypeCode = txtTypeCode.text;
                        AppConfig.RecentConnection.SelcalCode = txtSelcal.text;
                        AppConfig.saveConfig();
                        closeWindow()
                    }
                }
            }
        }

        Item {
            id: btnCancel
            Layout.preferredHeight: 30
            Layout.fillHeight: true
            Layout.columnSpan: 1
            Layout.row: 3
            Layout.column: 2
            Layout.fillWidth: true

            GrayButton {
                text: qsTr("Cancel")
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottomMargin: 0
                anchors.topMargin: 0
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                font.pixelSize: 13
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        closeWindow()
                    }
                }
            }
        }
    }
}
