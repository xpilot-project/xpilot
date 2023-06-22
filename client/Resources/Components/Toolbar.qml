import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

import org.vatsim.xpilot
import "../Controls"

GridLayout {
    id: toolbar
    columns: 2
    anchors.fill: parent
    property bool simConnected: false
    property bool networkConnected: false
    property string myCallsign: ""
    property bool showAfterModelDownloadDialog: false

    Connections {
        target: xplaneAdapter

        function onSimConnectionStateChanged(state) {
            simConnected = state;
            if(!state) {
                btnIdent.isActive = false;
                btnModeC.isActive = false;
            }
        }

        function onRadioStackStateChanged(stack) {
            btnIdent.isActive = stack.SquawkingIdent;
            btnModeC.isActive = stack.SquawkingModeC;
        }
    }

    Connections {
        target: networkManager

        function onNetworkConnected(callsign) {
            networkConnected = true;
            myCallsign = callsign
        }

        function onNetworkDisconnected() {
            networkConnected = false
        }
    }

    Connections {
        target: downloadCslWindow
        function onCloseWindow() {
            if(showAfterModelDownloadDialog) {
                connectWindowDialog.open()
                showAfterModelDownloadDialog = false
            }
        }
    }

    Row {
        leftPadding: 10
        spacing: 5

        ToolbarButton {
            id: btnConnect
            text: networkConnected ? "Disconnect" : "Connect"
            enabled: simConnected
            active: networkConnected
            MouseArea {
                id: btnConnectMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if(connectWindowDialog.visible) {
                        return
                    }
                    if(networkConnected) {
                        networkManager.disconnectFromNetwork();
                    }
                    else {
                        if(AppConfig.configRequired()) {
                            configRequiredDialog.open()
                        }
                        else {
                            if(!AppConfig.SilenceModelInstall) {
                                createCslDownloadWindow()
                                showAfterModelDownloadDialog = true
                            }
                            else {
                                connectWindowDialog.open()
                            }
                        }
                    }
                }
            }
        }

        TransponderButton {
            id: btnModeC
            text: "Mode C"
            enabled: simConnected
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    xplaneAdapter.transponderModeToggle();
                }
            }
        }

        TransponderButton {
            id: btnIdent
            text: "Ident"
            enabled: simConnected
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    xplaneAdapter.transponderIdent();
                }
            }
        }

        ToolbarButton {
            id: btnFlightPlan
            text: "Flight Plan"
            MouseArea {
                id: btnFlightPlanMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    Qt.openUrlExternally("https://my.vatsim.net/pilots/flightplan")
                }
            }
        }

        ToolbarButton {
            id: btnSettings
            text: "Settings"
            MouseArea {
                id: btnSettingsMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    settingsWindowDialog.show()
                }
            }
        }

        Text {
            id: myCallsignLabel
            text: myCallsign
            font.family: robotoMono.name
            font.pixelSize: 16
            renderType: Text.NativeRendering
            color: "white"
            leftPadding: 5
            height: 30
            verticalAlignment: Text.AlignVCenter
            visible: networkConnected
        }
    }

    Row {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        rightPadding: 5
        layoutDirection: Qt.RightToLeft
        spacing: -10

        WindowControlButton {
            id: btnClose
            icon.source: "../Icons/CloseIcon.svg"
            icon.color: "transparent"
            icon.width: 20
            icon.height: 20
            onHoveredChanged: hovered ? icon.color = "white" : icon.color = "transparent"

            MouseArea {
                id: btnCloseMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onClicked: {
                    mainWindow.close()
                }
            }
        }

        WindowControlButton {
            id: btnMinimize
            icon.source: "../Icons/MinimizeIcon.svg"
            icon.color: "transparent"
            icon.width: 20
            icon.height: 20
            onHoveredChanged: hovered ? icon.color = "white" : icon.color = "transparent"

            MouseArea {
                id: btnMinimizeMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    mainWindow.visibility = "Minimized"
                }
            }
        }
    }
}
