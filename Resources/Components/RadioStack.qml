import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import "../Controls"
import "../Scripts/FrequencyUtils.js" as FrequencyUtils

ColumnLayout {
    property bool avionicsPower: false

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

    Connections {
        target: udpClient

        function onAvionicsPowerOnChanged(power) {
            avionicsPower = power;
        }

        function onAudioComSelectionChanged(radio) {
            isCom1TxEnabled = radio === 6;
            isCom2TxEnabled = radio === 7;
        }

        function onCom1AudioSelectionChanged(active) {
            isCom1RxEnabled = active;
        }

        function onCom2AudioSelectionChanged(active) {
            isCom2RxEnabled = active;
        }

        function onCom1FrequencyChanged(freq) {
            com1Frequency = FrequencyUtils.printFrequency(freq);
        }

        function onCom2FrequencyChanged(freq) {
            com2Frequency = FrequencyUtils.printFrequency(freq);
        }
    }

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
            font.bold: true
            font.family: robotoMono.name
            renderType: Text.NativeRendering
        }
        Text {
            id: com1FreqLabel
            text: avionicsPower ? com1Frequency : "---.---"
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            leftPadding: 5
            font.pixelSize: 16
            font.family: robotoMono.name
            renderType: Text.NativeRendering
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 15
        }
        RadioStackIndicator{
            id: com1Tx
            isEnabled: avionicsPower && isCom1TxEnabled
            isActive: avionicsPower && isCom1Tx
            label: "TX"
        }
        Text {
            rightPadding: 5
        }
        RadioStackIndicator{
            id: com1Rx
            isEnabled: avionicsPower && isCom1RxEnabled
            isActive: avionicsPower && isCom1Rx
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
            font.bold: true
            font.family: robotoMono.name
            renderType: Text.NativeRendering
        }
        Text {
            id: com2FreqLabel
            text: avionicsPower ? com2Frequency : "---.---"
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            leftPadding: 5
            font.pixelSize: 16
            font.family: robotoMono.name
            renderType: Text.NativeRendering
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 15
        }
        RadioStackIndicator{
            id: com2Tx
            isEnabled: avionicsPower && isCom2TxEnabled
            isActive: avionicsPower && isCom2Tx
            label: "TX"
        }
        Text {
            rightPadding: 5
        }
        RadioStackIndicator{
            id: com2Rx
            isEnabled: avionicsPower && isCom2RxEnabled
            isActive: avionicsPower && isCom2Rx
            label: "RX"
        }
    }
}
