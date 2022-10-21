import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import AppConfig 1.0
import "../Components"
import "../Controls"

Popup {
    id: popup
    width: 500
    height: 180
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    focus: true
    closePolicy: Popup.NoAutoClose
    background: Rectangle {
        color: "white"
        border.color: "black"
    }

    signal closeWindow()

    function trimLineBreaks(value) {
        return value.replace(/[\n\r]/g, "")
    }

    MouseArea {
        anchors.fill: parent
        onClicked: typeCodeResults.visible = false
    }

    InvalidTypeCodeDialog {
        id: invaidTypeCodeDialog

        onUseInvalidTypeCode: {
            connectToNetwork()
        }
    }

    Connections {
        target: typeCodeDatabase

        function onTypeCodeResults(results) {
            typeCodeList.clear()
            for(var i = 0; i < results.length; i++) {
                var item = results[i];
                typeCodeList.append({name: item.name, icao: item.typeCode, manufacturer: item.manufacturer});
            }
            typeCodeResults.visible = typeCodeList.count > 0
        }

        function onValidateTypeCode(valid) {
            if(valid) {
                connectToNetwork()
            }
            else {
                invaidTypeCodeDialog.open()
            }
        }
    }

    Component.onCompleted: {
        txtCallsign.text = trimLineBreaks(AppConfig.RecentConnection.Callsign);
        txtTypeCode.text = trimLineBreaks(AppConfig.RecentConnection.TypeCode);
        txtSelcal.text = trimLineBreaks(AppConfig.RecentConnection.SelcalCode);
    }

    function connectToNetwork() {
        networkManager.connectToNetwork(trimLineBreaks(txtCallsign.text), trimLineBreaks(txtTypeCode.text), trimLineBreaks(txtSelcal.text), observerMode.checked);
        AppConfig.RecentConnection.Callsign = trimLineBreaks(txtCallsign.text);
        AppConfig.RecentConnection.TypeCode = trimLineBreaks(txtTypeCode.text);
        AppConfig.RecentConnection.SelcalCode = trimLineBreaks(txtSelcal.text);
        AppConfig.saveConfig();
        closeWindow()
    }

    ListModel {
        id: typeCodeList
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
            z: 100

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
                Keys.onReleased: function(event) {
                    if(event.key === Qt.Key_Escape) {
                        typeCodeResults.visible = false
                    }
                    else {
                        if(txtTypeCode.text.length > 0) {
                            typeCodeDatabase.searchTypeCodes(txtTypeCode.text.toUpperCase());
                        }
                        else {
                            typeCodeResults.visible = false
                        }
                    }
                }
            }

            Rectangle {
                id: typeCodeResults
                visible: false
                anchors.top: txtTypeCode.bottom
                anchors.topMargin: 2
                width: 300
                height: 120
                color: "white"
                border.color: "#BABFC4"
                border.width: 1
                clip: true

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.ArrowCursor
                    hoverEnabled: true
                }

                ListView {
                    id: typeCodeListView
                    currentIndex: -1
                    anchors.fill: parent
                    highlight: Rectangle { color: "lightsteelblue"; }
                    model: typeCodeList
                    delegate: Component {
                        id: contactDelegate
                        Item {
                            width: typeCodeResults.width
                            height: 25
                            Column {
                                Text {
                                    text: `<b>${icao}</b>: ${manufacturer} ${name}`
                                    leftPadding: 5
                                    topPadding: 5
                                    color: "#333333"
                                    renderType: Text.NativeRendering
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 13
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onEntered: typeCodeListView.currentIndex = index
                                onClicked: {
                                    typeCodeListView.currentIndex = index
                                    txtTypeCode.text = typeCodeList.get(typeCodeListView.currentIndex).icao
                                    typeCodeResults.visible = false
                                }
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { active: true }
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
                        if(txtCallsign.text === "") {
                            validationPopup.errorMessage = "Callsign is required"
                            validationPopup.open()
                            return
                        }
                        if(txtTypeCode.text === "") {
                            validationPopup.errorMessage = "Aircraft type code is required"
                            validationPopup.open()
                            return
                        }
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

                        typeCodeDatabase.validateTypeCodeBeforeConnect(txtTypeCode.text.toUpperCase())
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
