import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs

import org.vatsim.xpilot
import "../Components"
import "../Controls"
import "../Views/Settings"

Window {
    id: settingsWindow
    title: "xPilot Settings"
    width: 850
    height: 475
    color: "#ffffff"
    minimumHeight: height
    minimumWidth: width
    maximumHeight: height
    maximumWidth: width
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint
    modality: Qt.ApplicationModal

    property bool serverListLoaded: false
    property bool initialized: false
    property bool cancelButton: false

    signal closeWindow()

    onAfterRendering: {
        if(!initialized) {
            initialized = true
            audio.settingsWindowOpened()
        }
    }

    onVisibleChanged: {
        if(visible) {
            btnApply.enabled = false
            navigation.currentIndex = 0
        }
    }

    onClosing: function(close) {
        if (!cancelButton && audioSettings.inputDeviceChanged) {
            calibrationRequired.open()
            close.accepted = false
        }
        else {
            closeWindow()
            audio.settingsWindowClosed()
        }
    }

    function trimLineBreaks(value) {
        return value.replace(/[\n\r]/g, "")
    }

    Popup {
        id: calibrationRequired
        width: 520
        height: 220
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
            text: "<strong>Microphone Calibration Required</strong><br/><br/>Due to a change in your microphone device, it's important to ensure that your microphone volume is set to an appropriate level.<br/><br/>Please ensure that the microphone volume indicator stays within the green zone while speaking in a typical conversational tone. Utilize the Mic Volume slider to make any necessary adjustments to the microphone volume."
            renderType: Text.NativeRendering
            font.pixelSize: 13
            width: parent.width
            wrapMode: Text.Wrap
            verticalAlignment: Text.AlignVCenter
            bottomPadding: 10
            leftPadding: 10
        }

        BlueButton {
            id: btnOK
            text: "OK"
            anchors.top: calibrationInfo.bottom
            width: 50
            height: 28
            x: 10
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    calibrationRequired.close()
                    audioSettings.inputDeviceChanged = false
                    AppConfig.MicrophoneCalibrated = true
                }
            }
        }
    }

    Rectangle {
        id: container
        width: parent.width
        height: parent.height
        anchors.centerIn: parent
        color: "transparent"

        VerticalTabBar {
            id: navigation
            currentIndex: 0
            spacing: 0
            width: 160
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.top: parent.top

            VerticalTabButton {
                text: "Network"
            }

            VerticalTabButton {
                text: "Notifications"
            }

            VerticalTabButton {
                text: "Audio"
            }

            VerticalTabButton {
                text: "Miscellaneous"
            }
        }

        StackLayout {
            id: viewPanel
            width: parent.width
            height: parent.height
            anchors.top: container.top
            anchors.topMargin: 10
            anchors.left: navigation.right
            anchors.leftMargin: 15
            currentIndex: navigation.currentIndex

            SettingsNetwork {
                id: networkSettings
                onApplyChanges: btnApply.enabled = true
            }

            SettingsNotifications {
                id: notificationSettings
                onApplyChanges: btnApply.enabled = true
            }

            SettingsAudio {
                id: audioSettings
                onApplyChanges: btnApply.enabled = true
            }

            SettingsMiscellaneous {
                id: miscSettings
                onApplyChanges: btnApply.enabled = true
            }
        }

        RowLayout {
            spacing: 10
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.bottomMargin: 15
            anchors.rightMargin: 15

            StandardButton {
                id: btnOk
                text: "OK"
                onClicked: function() {
                    AppConfig.applySettings()
                    AppConfig.saveConfig()
                    close()
                }
            }

            StandardButton {
                id: btnApply
                text: "Apply"
                enabled: false
                onClicked: function() {
                    AppConfig.applySettings()
                    enabled = false
                }
            }

            StandardButton {
                id: btnCancel
                text: "Cancel"
                onClicked: function() {
                    cancelButton = true
                    close()
                }
            }
        }
    }
}
