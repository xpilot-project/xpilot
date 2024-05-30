import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.vatsim.xpilot
import "../../Controls"

Item {

    signal applyChanges()

    property bool audioDeviceListLoaded: false

    Component.onCompleted: {
        switchAlertPrivateMessage.checked = AppConfig.AlertPrivateMessage
        switchAlertRadioMessage.checked = AppConfig.AlertRadioMessage
        switchAlertDirectRadioMessage.checked = AppConfig.AlertDirectRadioMessage
        switchAlertBroadcast.checked = AppConfig.AlertNetworkBroadcast
        switchAlertSelcal.checked = AppConfig.AlertSelcal
        switchAlertDisconnect.checked = AppConfig.AlertDisconnect
        notificationAudioDevice.model = notificationSoundEngine.AudioDevices
    }

    Connections {
        target: notificationSoundEngine

        function onOutputDevicesChanged() {
            notificationAudioDevice.model = notificationSoundEngine.AudioDevices
        }
    }

    ColumnLayout {
        spacing: 10
        width: 500

        CustomSwitch {
            id: switchAlertPrivateMessage
            text: "Alert when new private message is received"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.AlertPrivateMessage = switchAlertPrivateMessage.checked
                applyChanges()
            }
        }

        CustomSwitch {
            id: switchAlertRadioMessage
            text: "Alert when radio message is received"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.AlertRadioMessage = switchAlertRadioMessage.checked
                applyChanges()
            }
        }

        CustomSwitch {
            id: switchAlertDirectRadioMessage
            text: "Alert when direct radio message is received"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.AlertDirectRadioMessage = switchAlertDirectRadioMessage.checked
                applyChanges()
            }
        }

        CustomSwitch {
            id: switchAlertBroadcast
            text: "Alert when network broadcast message is received"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.AlertNetworkBroadcast = switchAlertBroadcast.checked
                applyChanges()
            }
        }

        CustomSwitch {
            id: switchAlertSelcal
            text: "Alert when SELCAL notification is received"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.AlertSelcal = switchAlertSelcal.checked
                applyChanges()
            }
        }

        CustomSwitch {
            id: switchAlertDisconnect
            text: "Alert when disconnected from network"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.AlertDisconnect = switchAlertDisconnect.checked
                applyChanges()
            }
        }

        CustomComboBox {
            id: notificationAudioDevice
            fieldLabel: "Notification Audio Device"
            valueRole: "name"
            textRole: "name"
            onModelChanged: {
                currentIndex = indexOfValue(AppConfig.NotificationAudioDevice)
                audioDeviceListLoaded = true
            }
            onSelectedValueChanged: function(value) {
                if(audioDeviceListLoaded) {
                    notificationSoundEngine.setNotificationAudioDevice(value)
                    AppConfig.NotificationAudioDevice = value
                    applyChanges()
                }
            }
        }
    }
}
