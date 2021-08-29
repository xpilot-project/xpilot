import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import "../Controls"

ColumnLayout {
    property var com1Frequency: "---.---"
    property var com2Frequency: "---.---"

    property var isCom1TxEnabled: false
    property var isCom2TxEnabled: false
    property var isCom1RxEnabled: false
    property var isCom2RxEnabled: false

    property var isCom1Rx: false
    property var isCom2Rx: false

    property var isCom1Tx: false
    property var isCom2Tx: false

    Row {
        id: com1
        spacing: 0
        Layout.leftMargin: 0
        Layout.topMargin: 10
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.preferredHeight: 35

        Text {
            id: com1Label
            text: qsTr("COM1:")
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            font.pixelSize: 16
            font.weight: Font.DemiBold
            font.bold: false
            font.family: robotoMono.name
        }
        Text {
            id: com1FreqLabel
            text: com1Frequency
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            leftPadding: 5
            font.pixelSize: 16
            font.weight: Font.DemiBold
            font.bold: false
            font.family: robotoMono.name
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 15
        }
        RadioStackIndicator{
            id: com1Tx
            isEnabled: isCom1TxEnabled
            isActive: isCom1Tx
            label: "TX"
        }
        Text {
            rightPadding: 5
        }
        RadioStackIndicator{
            id: com1Rx
            isEnabled: isCom1RxEnabled
            isActive: isCom1Rx
            label: "RX"
        }
    }
    // com2
    Row {
        id: com2
        spacing: 0
        Layout.leftMargin: 0
        Layout.bottomMargin: 10
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.preferredHeight: 35

        Text {
            id: com2Label
            text: qsTr("COM2:")
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            font.pixelSize: 16
            font.weight: Font.DemiBold
            font.bold: false
            font.family: robotoMono.name
        }
        Text {
            id: com2FreqLabel
            text: com2Frequency
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            leftPadding: 5
            font.pixelSize: 16
            font.weight: Font.DemiBold
            font.bold: false
            font.family: robotoMono.name
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 15
        }
        RadioStackIndicator{
            id: com2Tx
            isEnabled: isCom2TxEnabled
            isActive: isCom2Tx
            label: "TX"
        }
        Text {
            rightPadding: 5
        }
        RadioStackIndicator{
            id: com2Rx
            isEnabled: isCom2TxEnabled
            isActive: isCom2Rx
            label: "RX"
        }
    }
}
