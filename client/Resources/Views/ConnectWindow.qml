import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs

import org.vatsim.xpilot
import "../Components"
import "../Controls"

Popup {
    id: popup
    width: 500
    height: 200
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    focus: true
    modal: true
    closePolicy: Popup.NoAutoClose

    property string errorMessage: ""

    background: Rectangle {
        color: "white"
        border.color: "black"
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
            for (var i = 0; i < results.length; i++) {
                var item = results[i]
                typeCodeList.append({ name: item.name, icao: item.typeCode, manufacturer: item.manufacturer })
            }
            typeCodeResults.visible = typeCodeList.count > 0
        }

        function onValidateTypeCode(valid) {
            if (valid) {
                connectToNetwork()
            }
            else {
                invaidTypeCodeDialog.open()
            }
        }
    }

    Component.onCompleted: {
        txtCallsign.fieldValue = AppConfig.RecentConnection.Callsign
        txtTypeCode.fieldValue = AppConfig.RecentConnection.TypeCode
        txtSelcal.fieldValue = AppConfig.RecentConnection.SelcalCode
    }

    function connectToNetwork() {
        networkManager.connectToNetwork(txtCallsign.fieldValue, txtTypeCode.fieldValue, txtSelcal.fieldValue, observerMode.checked)
        AppConfig.RecentConnection.Callsign = txtCallsign.fieldValue
        AppConfig.RecentConnection.TypeCode = txtTypeCode.fieldValue
        AppConfig.RecentConnection.SelcalCode = txtSelcal.fieldValue
        AppConfig.saveConfig()
        close()
    }

    ListModel {
        id: typeCodeList
    }

    GridLayout {
        anchors.fill: parent
        anchors.rightMargin: 30
        anchors.leftMargin: 30
        anchors.bottomMargin: 15
        anchors.topMargin: 15
        columns: 3
        rows: 4

        CustomTextField {
            id: txtCallsign
            Layout.column: 0
            Layout.row: 1
            fieldLabel: "Callsign"
            maximumLength: 10
            isUppercase: true
            validator: RegularExpressionValidator {
                regularExpression: /^[a-zA-Z0-9]+$/
            }
        }

        Item {
            id: typeCodeContainer
            Layout.preferredHeight: 35
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 0
            Layout.column: 1
            Layout.row: 1
            z: 100

            CustomTextField {
                id: txtTypeCode
                height: 28
                width: typeCodeContainer.width
                anchors.fill: typeCodeContainer
                fieldLabel: "Type Code"
                maximumLength: observerMode.checked ? 8 : 7
                isUppercase: true
                Keys.onReleased: function(event) {
                    if (event.key === Qt.Key_Escape) {
                        typeCodeResults.visible = false
                    }
                    else {
                        if (txtTypeCode.fieldValue.length > 0) {
                            typeCodeDatabase.searchTypeCodes(txtTypeCode.fieldValue)
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
                    highlight: Rectangle {
                        color: "lightsteelblue"
                    }
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
                                    txtTypeCode.fieldValue = typeCodeList.get(typeCodeListView.currentIndex).icao
                                    typeCodeResults.visible = false
                                }
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { active: true }
                }
            }
        }

        CustomTextField {
            id: txtSelcal
            Layout.column: 2
            Layout.row: 1
            fieldLabel: "SELCAL"
            isUppercase: true
            validator: RegularExpressionValidator {
                regularExpression: /^[A-Za-z-]+$/
            }
        }

        Item {
            Layout.preferredHeight: 40
            Layout.fillHeight: true
            Layout.column: 0
            Layout.row: 2
            Layout.columnSpan: 3
            Layout.fillWidth: true

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
                    txtCallsign.fieldValue = observerMode.checked ? txtCallsign.fieldValue.substring(0, 8) : txtCallsign.fieldValue.substring(0, 7)
                }
            }
        }

        Item {
            Layout.preferredHeight: 40
            Layout.fillHeight: true
            Layout.column: 0
            Layout.row: 3
            Layout.columnSpan: 3
            Layout.fillWidth: true
            visible: errorMessage !== ""

            Text {
                text: errorMessage
                color: "#ff0000"
            }
        }

        Item {
            id: connectButton
            Layout.preferredHeight: 40
            Layout.fillHeight: true
            Layout.columnSpan: 2
            Layout.row: 4
            Layout.column: 0
            Layout.fillWidth: true

            BlueButton {
                id: blueButton
                text: "Connect to VATSIM"
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
                        if (txtCallsign.fieldValue === "") {
                            errorMessage = "Callsign is required"
                            return
                        }
                        if (txtTypeCode.fieldValue === "") {
                            errorMessage = "Aircraft type code is required"
                            return
                        }
                        if (txtSelcal.fieldValue !== "") {
                            const prohibitedChars = ["I", "N", "O"]
                            var selcal = txtSelcal.fieldValue
                            selcal = selcal.replace("-", "")
                            selcal = selcal.toUpperCase()
                            const chars = selcal.split("")
                            if (/([a-zA-Z]).*?\1/.test(selcal)) {
                                errorMessage = "SELCAL cannot contain repeating characters."
                                return
                            }
                            for (let i = 1; i < chars.length; i += 2) {
                                if (chars[i] > 'S') {
                                    errorMessage = "SELCAL can only contain characters A through S."
                                    return
                                }
                                if (chars[i - 1] > 'R') {
                                    errorMessage = "SELCAL can only contain characters A through S."
                                    return
                                }
                                if (prohibitedChars.includes(chars[i])) {
                                    errorMessage = "SELCAL cannot contain characters I, N or O."
                                    return
                                }
                                if (chars[i] < chars[i - 1]) {
                                    errorMessage = "SELCAL characters must be in ascending, alphabetical order."
                                    return
                                }
                            }
                        }

                        errorMessage = ""
                        typeCodeDatabase.validateTypeCodeBeforeConnect(txtTypeCode.fieldValue)
                    }
                }
            }
        }

        Item {
            id: btnCancel
            Layout.preferredHeight: 30
            Layout.fillHeight: true
            Layout.columnSpan: 1
            Layout.row: 4
            Layout.column: 2
            Layout.fillWidth: true

            GrayButton {
                text: "Cancel"
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
                        close()
                    }
                }
            }
        }
    }
}
