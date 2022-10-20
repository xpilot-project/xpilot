import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs

import "../Components"
import "../Controls"
import AppConfig 1.0

Window {
    id: formSettings
    title: "Settings"
    width: 670
    height: 615
    minimumHeight: height
    minimumWidth: width
    maximumHeight: height
    maximumWidth: width
    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    property bool serverListLoaded: false
    property bool inputDeviceListLoaded: false
    property bool outputDeviceListLoaded: false
    property bool initialized: false
    property bool inputDeviceChanged: false

    signal closeWindow()

    function trimLineBreaks(value) {
        return value.replace(/[\n\r]/g, "")
    }

    onClosing: (close) => {
        if(inputDeviceChanged && inputDeviceList.currentIndex > -1) {
            close.accepted = false
            calibrationRequired.open()
        }
        else {
            AppConfig.saveConfig();
            closeWindow()
        }
        audio.settingsWindowClosed()
    }

    Connections {
        target: audio

        function onInputDevicesChanged() {
            if(inputDeviceListLoaded) {
                inputDeviceList.model = audio.InputDevices;
                inputDeviceList.currentIndex = -1;
            }
        }

        function onOutputDevicesChanged() {
            if(outputDeviceListLoaded) {
                outputDeviceList.model = audio.OutputDevices;
                outputDeviceList.currentIndex = -1;
            }
        }

        function onInputVuChanged(vu) {
            peakLevel.value = vu
        }
    }

    Component.onCompleted: {
        txtVatsimId.text = AppConfig.VatsimId;
        txtVatsimPassword.text = AppConfig.VatsimPasswordDecrypted;
        txtYourName.text = AppConfig.Name;
        txtHomeAirport.text = AppConfig.HomeAirport;
        networkServerCombobox.model = AppConfig.CachedServers;
        outputDeviceList.model = audio.OutputDevices;
        inputDeviceList.model = audio.InputDevices;
        com1Slider.volume = AppConfig.Com1Volume;
        com2Slider.volume = AppConfig.Com2Volume;
        microphoneVolume.volume = AppConfig.MicrophoneVolume;
        switchEnableHfSquelch.checked = AppConfig.HFSquelchEnabled;
        switchDisableRadioEffects.checked = AppConfig.AudioEffectsDisabled;
        switchAutoModeC.checked = AppConfig.AutoModeC;
        switchAlertBroadcast.checked = AppConfig.AlertNetworkBroadcast;
        switchAlertRadioMessage.checked = AppConfig.AlertRadioMessage;
        switchAlertDirectRadioMessage.checked = AppConfig.AlertDirectRadioMessage;
        switchAlertPrivateMessage.checked = AppConfig.AlertPrivateMessage;
        switchAlertSelcal.checked = AppConfig.AlertSelcal;
        switchAlertDisconnect.checked = AppConfig.AlertDisconnect;
        switchKeepWindowVisible.checked = AppConfig.KeepWindowVisible
        switchAircraftVolumeKnobs.checked = AppConfig.AircraftRadioStackControlsVolume
    }

    onAfterRendering: {
        if(!initialized) {
            // prevent window from opening outside of screen bounds
            if((y - 50) < screen.virtualY) {
                y = screen.virtualY + 50
            }
            initialized = true
            audio.settingsWindowOpened()
        }
    }

    Popup {
        id: calibrationRequired
        width: 500
        height: 200
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose
        margins: 20
        background: Rectangle {
            color: "white"
            border.color: "black"
        }

        Text {
            id: calibrationInfo
            text: "<strong>Microphone Calibration Required</strong><br/><br/>Because your microphone device changed, you must confirm that your microphone volume is at an acceptable level.<br/><br/>Please verify that the microphone volume level indicator stays green when you speak normally. Use the Mic Volume slider to adjust the microphone volume as necessary."
            font.pixelSize: 14
            renderType: Text.NativeRendering
            width: parent.width
            wrapMode: Text.Wrap
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
            anchors.top: calibrationInfo.bottom
            x: 10
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    calibrationRequired.close()
                    inputDeviceChanged = false
                    AppConfig.MicrophoneCalibrated = true
                }
            }
        }
    }


    GridLayout {
        id: gridLayout
        anchors.fill: parent
        anchors.rightMargin: 15
        anchors.leftMargin: 15
        anchors.bottomMargin: 15
        anchors.topMargin: 15
        columnSpacing: 15
        rowSpacing: 10
        rows: 8
        columns: 2

        Item {
            id: vatsimId
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 0
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50
            Label {
                color: "#333333"
                text: qsTr("VATSIM ID")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }
            CustomTextField {
                id: txtVatsimId
                onTextChanged: {
                    AppConfig.VatsimId = trimLineBreaks(txtVatsimId.text.trim())
                }
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]+/
                }
                y: 20
            }
        }

        Item {
            id: vatsimPassword
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 1
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50
            Label {
                color: "#333333"
                text: qsTr("VATSIM Password")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }
            CustomTextField {
                id: txtVatsimPassword
                echoMode: TextInput.Password
                y: 20
                onTextChanged: {
                    AppConfig.VatsimPasswordDecrypted = trimLineBreaks(txtVatsimPassword.text.trim())
                }
            }
        }

        Item {
            id: yourName
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50
            Label {
                color: "#333333"
                text: qsTr("Your Name")
                renderType: Text.NativeRendering
                font.pixelSize: 13
            }

            CustomTextField {
                id: txtYourName
                y: 20
                onTextChanged: {
                    AppConfig.Name = trimLineBreaks(txtYourName.text.trim());
                }
            }
        }


        Item {
            id: homeAirport
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 3
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50
            Label {
                color: "#333333"
                text: qsTr("Home Airport")
                renderType: Text.NativeRendering
                font.pixelSize: 13
            }
            CustomTextField {
                id: txtHomeAirport
                y: 20
                onTextChanged: {
                    txtHomeAirport.text = txtHomeAirport.text.toUpperCase()
                    AppConfig.HomeAirport = trimLineBreaks(txtHomeAirport.text.trim())
                }
                validator: RegularExpressionValidator {
                    regularExpression: /[a-zA-Z0-9]{4}/
                }
            }
        }

        Item {
            id: networkServer
            Layout.preferredHeight: 50
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.preferredWidth: 50
            Layout.column: 0
            Layout.row: 4
            Label {
                id: networkServerLabel
                color: "#333333"
                text: qsTr("VATSIM Server")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }
            CustomComboBox {
                id: networkServerCombobox
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: networkServerLabel.bottom
                anchors.topMargin: 5
                anchors.leftMargin: 0
                anchors.rightMargin: 0
                textRole: "name"
                valueRole: "address"
                onModelChanged: {
                    currentIndex = find(AppConfig.ServerName);
                    serverListLoaded = true;
                }
                onCurrentIndexChanged: {
                    if(serverListLoaded) {
                        AppConfig.ServerName = networkServerCombobox.textAt(currentIndex)
                    }
                }
            }
        }

        Item {
            id: clientOptions
            Layout.fillHeight: true
            Layout.column: 1
            Layout.row: 0
            Layout.rowSpan: 5
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
            Layout.fillWidth: true

            ColumnLayout {
                id: columnLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.topMargin: 0
                anchors.bottomMargin: 0
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                spacing: 0

                CustomSwitch {
                    id: switchAutoModeC
                    text: "Automatically set transponder to Mode C on takeoff"
                    font.pixelSize: 13
                    clip: false
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.AutoModeC = switchAutoModeC.checked
                    }
                }

                CustomSwitch {
                    id: switchAlertPrivateMessage
                    text: "Alert when new private message is received"
                    font.pixelSize: 13
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.AlertPrivateMessage = switchAlertPrivateMessage.checked
                    }
                }

                CustomSwitch {
                    id: switchAlertRadioMessage
                    text: "Alert when radio message is received"
                    font.pixelSize: 13
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.AlertRadioMessage = switchAlertRadioMessage.checked
                    }
                }

                CustomSwitch {
                    id: switchAlertDirectRadioMessage
                    text: "Alert when direct radio message is received"
                    font.pixelSize: 13
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.AlertDirectRadioMessage = switchAlertDirectRadioMessage.checked
                    }
                }

                CustomSwitch {
                    id: switchAlertBroadcast
                    text: "Alert when network broadcast message is received"
                    font.pixelSize: 13
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.AlertNetworkBroadcast = switchAlertBroadcast.checked
                    }
                }

                CustomSwitch {
                    id: switchAlertSelcal
                    text: "Alert when SELCAL notification is received"
                    font.pixelSize: 13
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.AlertSelcal = switchAlertSelcal.checked
                    }
                }

                CustomSwitch {
                    id: switchAlertDisconnect
                    text: "Alert when disconnected from network"
                    font.pixelSize: 13
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.AlertDisconnect = switchAlertDisconnect.checked
                    }
                }

                CustomSwitch {
                    id: switchKeepWindowVisible
                    text: "Keep xPilot window visible"
                    font.pixelSize: 13
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        AppConfig.KeepWindowVisible = switchKeepWindowVisible.checked
                    }
                }

                CustomSwitch {
                    id: switchEnableHfSquelch
                    text: "Enable HF Squelch"
                    font.pixelSize: 13
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        audio.enableHfSquelch(switchEnableHfSquelch.checked)
                    }
                }

                CustomSwitch {
                    id: switchDisableRadioEffects
                    text: "Disable Realistic Radio Effects"
                    font.pixelSize: 13
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: 287
                    onCheckedChanged: {
                        audio.disableAudioEffects(switchDisableRadioEffects.checked)
                    }
                }
            }
        }

        Item {
            id: microphoneDevice
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.column: 0
            Layout.preferredWidth: 50
            Layout.row: 7
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Text {
                id: text7
                color: "#333333"
                text: qsTr("Microphone Device")
                renderType: Text.NativeRendering
                font.pixelSize: 13
            }

            CustomComboBox {
                id: inputDeviceList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: text7.bottom
                anchors.topMargin: 5
                anchors.leftMargin: 0
                anchors.rightMargin: 0
                textRole: "name"
                valueRole: "name"
                onModelChanged: {
                    currentIndex = inputDeviceList.indexOfValue(AppConfig.InputDevice)
                    inputDeviceListLoaded = true
                }
                onCurrentIndexChanged: {
                    if(inputDeviceListLoaded) {
                        var device = inputDeviceList.textAt(currentIndex)
                        AppConfig.InputDevice = device
                        audio.setInputDevice(device)
                        inputDeviceChanged = true
                    }
                }
            }
        }

        Item {
            id: listenDevice
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.column: 1
            Layout.row: 7
            Layout.preferredWidth: 50
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Text {
                id: text9
                color: "#333333"
                text: qsTr("Listen Device")
                renderType: Text.NativeRendering
                font.pixelSize: 13
            }

            CustomComboBox {
                id: outputDeviceList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: text9.bottom
                anchors.topMargin: 5
                anchors.leftMargin: 0
                anchors.rightMargin: 0
                textRole: "name"
                valueRole: "name"
                onModelChanged: {
                    currentIndex = outputDeviceList.indexOfValue(AppConfig.OutputDevice)
                    outputDeviceListLoaded = true
                }
                onCurrentIndexChanged: {
                    if(outputDeviceListLoaded) {
                        var device = outputDeviceList.textAt(currentIndex)
                        AppConfig.OutputDevice = device
                        audio.setOutputDevice(device)
                    }
                }
            }
        }


        Item {
            id: microphoneLevel
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.column: 0
            Layout.row: 8
            Layout.fillWidth: true

            ColumnLayout {
                id: columnLayout2
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: -10
                anchors.rightMargin: 0
                anchors.leftMargin: 0

                PeakLevelControl {
                    id: peakLevel
                    width: 310
                    height: 13
                }
            }

            ColumnLayout {
                id: microphoneVolumeGroup
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.rightMargin: 0
                anchors.leftMargin: 0

                Text {
                    id: microphoneVolumeInfo
                    text: "Adjust the mic volume slider so that the peak level indicator remains green while speaking normally."
                    renderType: Text.NativeRendering
                    wrapMode: Text.WordWrap
                    Layout.maximumWidth: 300
                    linkColor: "#0164AD"
                    font.pixelSize: 13
                    color: "#333333"
                    topPadding: 5
                }

                VolumeSlider {
                    id: microphoneVolume
                    comLabel: "Mic Volume"
                    minValue: -60
                    maxValue: 18
                    showPercent: false
                    onVolumeValueChanged: {
                        audio.setMicrophoneVolume(volume)
                        AppConfig.MicrophoneVolume = volume
                    }
                }

                Text {
                    id: name
                    text: "Your Push to Talk (PTT) must be assigned in X-Plane using the joystick or keyboard command bindings. <a href='https://vats.im/xpilot-ptt'>Learn more about how to set your PTT</a>"
                    onLinkActivated: Qt.openUrlExternally(link)
                    renderType: Text.NativeRendering
                    wrapMode: Text.WordWrap
                    Layout.maximumWidth: 600
                    linkColor: "#0164AD"
                    font.pixelSize: 13
                    color: "#333333"
                    topPadding: 5

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
            }
        }

        Item {
            id: volumeLevels
            height: 130
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.column: 1
            Layout.row: 8
            ColumnLayout {
                id: columnLayout4
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                spacing: 0
                anchors.topMargin: -10
                anchors.rightMargin: 0
                anchors.leftMargin: 0

                VolumeSlider {
                    id: com1Slider
                    comLabel: "COM1"
                    onVolumeValueChanged: function(volume) {
                        audio.setCom1Volume(volume)
                    }
                }

                VolumeSlider {
                    id: com2Slider
                    comLabel: "COM2"
                    onVolumeValueChanged: function(volume) {
                        audio.setCom2Volume(volume)
                    }
                }

                CustomSwitch {
                    id: switchAircraftVolumeKnobs
                    text: "Allow aircraft radio stack volume knobs to control (and override) radio volume"
                    font.pixelSize: 13
                    clip: false
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 300
                    leftPadding: 0
                    topPadding: 20
                    onCheckedChanged: {
                        AppConfig.AircraftRadioStackControlsVolume = switchAircraftVolumeKnobs.checked
                    }
                }
            }
        }
    }
}
