import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.vatsim.xpilot
import "../../Controls"

Item {

    signal applyChanges()

    property bool inputDeviceListLoaded: false
    property bool outputDeviceListLoaded: false
    property bool inputDeviceChanged: false

    Connections {
        target: audio

        function onInputDevicesChanged() {
            if(inputDeviceListLoaded) {
                inputDeviceList.model = audio.InputDevices
            }
        }

        function onOutputDevicesChanged() {
            if(outputDeviceListLoaded) {
                speakerDeviceList.model = audio.OutputDevices
                headsetDeviceList.model = audio.OutputDevices
            }
        }

        function onInputVuChanged(vu) {
            peakLevel.value = vu
        }
    }

    Connections {
        target: xplaneAdapter

        function onSplitAudioChannelsChanged(split) {
            switchSplitComChannels.checked = split
        }
    }

    Component.onCompleted: {
        inputDeviceList.model = audio.InputDevices
        headsetDeviceList.model = audio.OutputDevices
        speakerDeviceList.model = audio.OutputDevices
        switchSplitComChannels.checked = AppConfig.SplitAudioChannels
        switchEnableHfSquelch.checked = AppConfig.HFSquelchEnabled
        switchDisableRadioEffects.checked = AppConfig.AudioEffectsDisabled
        switchAircraftVolumeKnobs.checked = AppConfig.AircraftRadioStackControlsVolume
        com1Slider.volume = AppConfig.Com1Volume
        com2Slider.volume = AppConfig.Com2Volume
        microphoneVolume.volume = AppConfig.MicrophoneVolume
    }

    ColumnLayout {
        spacing: 10
        width: 500

        CustomComboBox {
            id: inputDeviceList
            fieldLabel: "Microphone Device:"
            valueRole: "name"
            textRole: "name"
            onModelChanged: {
                currentIndex = indexOfValue(AppConfig.InputDevice)
                inputDeviceListLoaded = true
            }
            onSelectedValueChanged: function(value) {
                if(inputDeviceListLoaded) {
                    AppConfig.InputDevice = value
                    audio.setInputDevice(value)
                    inputDeviceChanged = true
                    applyChanges()
                }
            }
        }

        CustomComboBox {
            id: headsetDeviceList
            fieldLabel: "Headset Device:"
            valueRole: "name"
            textRole: "name"
            onModelChanged: {
                currentIndex = indexOfValue(AppConfig.HeadsetDevice)
                outputDeviceListLoaded = true
            }
            onSelectedValueChanged: function(value) {
                if(outputDeviceListLoaded) {
                    AppConfig.HeadsetDevice = value
                    audio.setHeadsetDevice(value)
                    applyChanges()
                }
            }
        }

        CustomComboBox {
            id: speakerDeviceList
            fieldLabel: "Speaker Device:"
            valueRole: "name"
            textRole: "name"
            onModelChanged: {
                currentIndex = indexOfValue(AppConfig.SpeakerDevice)
                outputDeviceListLoaded = true
            }
            onSelectedValueChanged: function(value) {
                if(outputDeviceListLoaded) {
                    AppConfig.SpeakerDevice = value
                    audio.setSpeakerDevice(value)
                    applyChanges()
                }
            }
        }

        RowLayout {
            spacing: 40

            ColumnLayout {
                width: 300
                spacing: 0
                Layout.alignment: Qt.AlignTop

                CustomSwitch {
                    id: switchSplitComChannels
                    text: "Split Output Audio Channels"
                    font.pixelSize: 13
                    leftPadding: 0
                    tooltipText: "Split output audio to separate channels (COM1 = Left, COM2 = Right)"
                    onCheckedChanged: {
                        AppConfig.SplitAudioChannels = switchSplitComChannels.checked
                        audio.setSplitAudioChannels(switchSplitComChannels.checked)
                        applyChanges()
                    }
                }

                CustomSwitch {
                    id: switchEnableHfSquelch
                    text: "Enable HF Squelch"
                    font.pixelSize: 13
                    leftPadding: 0
                    onCheckedChanged: {
                        AppConfig.HFSquelchEnabled = switchEnableHfSquelch.checked
                        audio.enableHfSquelch(switchEnableHfSquelch.checked)
                        applyChanges()
                    }
                }

                CustomSwitch {
                    id: switchDisableRadioEffects
                    text: "Disable Realistic Radio Effects"
                    leftPadding: 0
                    font.pixelSize: 13
                    onCheckedChanged: {
                        AppConfig.AudioEffectsDisabled = switchDisableRadioEffects.checked
                        audio.disableAudioEffects(switchDisableRadioEffects.checked)
                        applyChanges()
                    }
                }

                CustomSwitch {
                    id: switchAircraftVolumeKnobs
                    text: "Allow aircraft radio stack volume knobs to control (and override) radio volume"
                    font.pixelSize: 13
                    Layout.maximumWidth: 300
                    leftPadding: 0
                    onCheckedChanged: {
                        applyChanges()
                        AppConfig.AircraftRadioStackControlsVolume = switchAircraftVolumeKnobs.checked
                    }
                }

                VolumeSlider {
                    id: com1Slider
                    comLabel: "COM1"
                    onVolumeValueChanged: function(volume) {
                        applyChanges()
                        AppConfig.Com1Volume = volume
                        audio.setCom1Volume(volume)
                    }
                }

                VolumeSlider {
                    id: com2Slider
                    comLabel: "COM2"
                    onVolumeValueChanged: function(volume) {
                        applyChanges()
                        AppConfig.Com2Volume = volume
                        audio.setCom2Volume(volume)
                    }
                }
            }

            ColumnLayout {
                width: 300
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 10
                Layout.rightMargin: 10

                PeakLevelControl {
                    id: peakLevel
                    height: 13
                }

                Text {
                    text: "Adjust the mic volume slider so that the peak level indicator remains green while speaking normally."
                    renderType: Text.NativeRendering
                    wrapMode: Text.Wrap
                    Layout.maximumWidth: 300
                    topPadding: 5
                    font.pixelSize: 13
                    color: "#000000"
                }

                VolumeSlider {
                    id: microphoneVolume
                    comLabel: "Mic Volume"
                    minValue: -60
                    maxValue: 18
                    showPercent: false
                    onVolumeValueChanged: function(volume) {
                        applyChanges()
                        audio.setMicrophoneVolume(volume)
                        AppConfig.MicrophoneVolume = volume
                    }
                }
            }
        }

        Text {
            id: name
            text: "Your Push to Talk (PTT) must be assigned in X-Plane using the joystick or keyboard command bindings. <a href='http://xpilot-project.org/ptt-setup'>Learn more about how to set your PTT</a>"
            onLinkActivated: (link) => Qt.openUrlExternally(link)
            renderType: Text.NativeRendering
            wrapMode: Text.WordWrap
            Layout.maximumWidth: parent.width
            linkColor: "#0164AD"
            font.pixelSize: 13
            color: "#000000"

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }
    }
}
