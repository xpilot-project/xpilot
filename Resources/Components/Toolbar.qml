import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import "../Controls"

GridLayout {
    id: toolbar
    columns: 2
    anchors.fill: parent
    property bool wsConnected: false

    signal toggleModeC(bool active)
    signal squawkIdent()

    Row {
        leftPadding: 15
        spacing: 5

        ToolbarButton {
            id: btnConnect
            text: "Connect"
            MouseArea {
                id: btnConnectMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var comp = Qt.createComponent("qrc:/Resources/Views/ConnectWindow.qml")
                    if(comp.status === Component.Ready) {
                        connectWindow = comp.createObject(mainWindow)
                        connectWindow.show()
                    }
                }
            }
        }

        TransponderButton {
            id: btnModeC
            text: "Mode C"
            enabled: wsConnected
            MouseArea {
                preventStealing: true
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    isModeC = !isModeC
                    btnModeC.isActive = isModeC
                    toggleModeC(isModeC)
                }
            }
        }

        TransponderButton {
            id: btnIdent
            text: "Ident"
            enabled: wsConnected
            MouseArea {
                preventStealing: true
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    squawkIdent()
                }
            }
        }

        ToolbarButton {
            id: btnFlightPlan
            text: "Flight Plan"
            enabled: wsConnected
            MouseArea {
                id: btnFlightPlanMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var comp = Qt.createComponent("qrc:/Resources/Views/FlightPlanWindow.qml")
                    if(comp.status === Component.Ready) {
                        flightPlanWindow = comp.createObject(mainWindow)
                        flightPlanWindow.show()
                    }
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
                    var comp = Qt.createComponent("qrc:/Resources/Views/SettingsWindow.qml")
                    if(comp.status === Component.Ready) {
                        settingsWindow = comp.createObject(mainWindow)
                        settingsWindow.show()
                    }
                }
            }
        }
    }

    Row {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        rightPadding: 15
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
