import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import org.vatsim.xpilot
import "../Controls"
import "../Scripts/FrequencyUtils.js" as FrequencyUtils

ColumnLayout {
    property bool simConnected: false
    property bool networkConnected: false

    property bool isCom1Rx: false
    property bool isCom2Rx: false

    property bool isCom1Tx: false
    property bool isCom2Tx: false

    property var radioStackState;

    property real com1Hf: 0
    property real com2Hf: 0

    property bool com1Headset: false
    property bool com2Headset: false

    Component.onCompleted: {
        com1Headset = AppConfig.Com1OnHeadset
        com2Headset = AppConfig.Com2OnHeadset
    }

    Connections {
        target: xplaneAdapter

        function onSimConnectionStateChanged(state) {
            simConnected = state;
        }

        function onRadioStackStateChanged(stack) {
            if(radioStackState !== stack) {
                radioStackState = stack;
            }
        }

        function onPttPressed() {
            if(radioStackState.Com1TransmitEnabled) {
                isCom1Tx = true
            }
            else if(radioStackState.Com2TransmitEnabled) {
                isCom2Tx = true
            }
        }

        function onPttReleased() {
            isCom1Tx = false
            isCom2Tx = false
        }

        function onCom1OnHeadsetChanged(onHeadset) {
            com1Headset = onHeadset
        }

        function onCom2OnHeadsetChanged(onHeadset) {
            com2Headset = onHeadset
        }
    }

    Connections {
        target: networkManager

        function onNetworkConnected() {
            networkConnected = true
        }

        function onNetworkDisconnected() {
            networkConnected = false
        }
    }

    Connections {
        target: audio

        function onRadioRxChanged(radio, state) {
            switch(radio) {
            case 0:
                isCom1Rx = state;
                break;
            case 1:
                isCom2Rx = state;
                break;
            }
        }

        function onRadioAliasChanged(radio, frequency) {
            switch(radio) {
            case 0:
                com1Hf = frequency / 1000000;
                break;
            case 1:
                com2Hf = frequency / 1000000;
                break;
            }
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
            leftPadding: 10
        }
        Text {
            id: com1FreqLabel
            text: simConnected && (radioStackState.AvionicsPowerOn || radioStackState.OverrideRadioPower) ? FrequencyUtils.printFrequency(radioStackState.Com1Frequency) : "---.---"
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            leftPadding: 5
            font.pixelSize: 16
            font.family: robotoMono.name
            renderType: Text.NativeRendering

            ToolTip.visible: com1MouseArea.containsMouse && com1Hf > 0
            ToolTip.text: "Com1 HF: " + com1Hf

            MouseArea {
                id: com1MouseArea
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
                onClicked: (mouse) => mouse.accepted = false
                onPressed: (mouse) => mouse.accepted = false
                onReleased: (mouse) => mouse.accepted = false
                onDoubleClicked: (mouse) => mouse.accepted = false
                onPositionChanged: (mouse) => mouse.accepted = false
                onPressAndHold: (mouse) => mouse.accepted = false
            }
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 10
        }
        RadioStackIndicator{
            id: com1Tx
            isEnabled: simConnected && (radioStackState.AvionicsPowerOn || radioStackState.OverrideRadioPower) && radioStackState.Com1TransmitEnabled
            isActive: networkConnected && isCom1Tx
            label: "TX"
        }
        Text {
            rightPadding: 5
        }
        RadioStackIndicator{
            id: com1Rx
            isEnabled: simConnected && (radioStackState.AvionicsPowerOn || radioStackState.OverrideRadioPower) && radioStackState.Com1ReceiveEnabled
            isActive: isCom1Rx
            label: "RX"
        }
        TransparentButton {
            icon.source: com1Headset ? "../Icons/HeadsetIcon.svg" : "../Icons/SpeakerIcon.svg"
            icon.color: "#141618"
            icon.width: 20
            icon.height: 20
            onHoveredChanged: hovered ? icon.color = "#ffffff" : icon.color = "#141618"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onClicked: {
                    com1Headset = !com1Headset
                    audio.setOnHeadset(0, com1Headset)
                }
            }
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
            leftPadding: 10
        }
        Text {
            id: com2FreqLabel
            text: simConnected && (radioStackState.AvionicsPowerOn || radioStackState.OverrideRadioPower) ? FrequencyUtils.printFrequency(radioStackState.Com2Frequency) : "---.---"
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            leftPadding: 5
            font.pixelSize: 16
            font.family: robotoMono.name
            renderType: Text.NativeRendering

            ToolTip.visible: com2MouseArea.containsMouse && com2Hf > 0
            ToolTip.text: "Com2 HF: " + com2Hf

            MouseArea {
                id: com2MouseArea
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
                onClicked: (mouse) => mouse.accepted = false
                onPressed: (mouse) => mouse.accepted = false
                onReleased: (mouse) => mouse.accepted = false
                onDoubleClicked: (mouse) => mouse.accepted = false
                onPositionChanged: (mouse) => mouse.accepted = false
                onPressAndHold: (mouse) => mouse.accepted = false
            }
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 10
        }
        RadioStackIndicator{
            id: com2Tx
            isEnabled: simConnected && (radioStackState.AvionicsPowerOn || radioStackState.OverrideRadioPower) && radioStackState.Com2TransmitEnabled
            isActive: networkConnected && isCom2Tx
            label: "TX"
        }
        Text {
            rightPadding: 5
        }
        RadioStackIndicator{
            id: com2Rx
            isEnabled: simConnected && (radioStackState.AvionicsPowerOn || radioStackState.OverrideRadioPower) && radioStackState.Com2ReceiveEnabled
            isActive: isCom2Rx
            label: "RX"
        }
        TransparentButton {
            icon.source: com2Headset ? "../Icons/HeadsetIcon.svg" : "../Icons/SpeakerIcon.svg"
            icon.color: "#141618"
            icon.width: 20
            icon.height: 20
            onHoveredChanged: hovered ? icon.color = "#ffffff" : icon.color = "#141618"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onClicked: {
                    com2Headset = !com2Headset
                    audio.setOnHeadset(1, com2Headset)
                }
            }
        }
    }
}
