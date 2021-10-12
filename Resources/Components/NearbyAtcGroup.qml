import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import "../Scripts/FrequencyUtils.js" as FrequencyUtils

Column {
    id: root

    property variant internalModel
    property string groupTitle
    signal startChatSession(string callsign)

    Label {
        color: "#6c757d"
        text: groupTitle
        font.pixelSize: 14
        font.family: robotoMono.name
        renderType: Text.NativeRendering
    }

    Repeater {
        model: internalModel
        Text {
            text: `${Callsign} - ${FrequencyUtils.formatNetwork(Frequency)}`
            font.pixelSize: 14
            font.family: robotoMono.name
            leftPadding: 15
            color: "#ffffff"
            renderType: Text.NativeRendering

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton | Qt.LeftButton
                cursorShape: Qt.PointingHandCursor

                onDoubleClicked: {
                    networkManager.requestControllerAtis(Callsign)
                }

                onClicked: {
                    if(mouse.button === Qt.RightButton) {
                        ctxtMenu.popup()
                    }
                }
                onPressAndHold: {
                    if(mouse.source === Qt.MouseEventNotSynthesized) {
                        ctxtMenu.popup()
                    }
                }

                Menu {
                    id: ctxtMenu
                    MenuItem {
                        hoverEnabled: false
                        enabled: false
                        contentItem: Text {
                            text: callsign
                            font.pixelSize: 14
                            font.family: robotoMono.name
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            renderType: Text.NativeRendering
                            topPadding: 5
                            color: "#565E64"
                        }
                        height: 30
                    }
                    MenuSeparator{}
                    MenuItem {
                        contentItem: Text {
                            text: "Send Private Message"
                            font.pixelSize: 13
                            font.family: robotoMono.name
                            verticalAlignment: Text.AlignVCenter
                            renderType: Text.NativeRendering
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: startChatSession(Callsign)
                            }
                        }
                        height: 35
                    }
                    MenuItem {
                        contentItem: Text {
                            text: "Request Controller Info"
                            font.pixelSize: 13
                            font.family: robotoMono.name
                            verticalAlignment: Text.AlignVCenter
                            renderType: Text.NativeRendering
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: networkManager.requestControllerAtis(Callsign)
                            }
                        }
                        height: 35
                    }
                    MenuItem {
                        contentItem: Text {
                            text: "Tune COM1 Radio"
                            font.pixelSize: 13
                            font.family: robotoMono.name
                            verticalAlignment: Text.AlignVCenter
                            renderType: Text.NativeRendering
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                            }
                        }
                        height: 35
                    }
                    MenuItem {
                        contentItem: Text {
                            text: "Tune COM2 Radio"
                            font.pixelSize: 13
                            font.family: robotoMono.name
                            verticalAlignment: Text.AlignVCenter
                            renderType: Text.NativeRendering
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                            }
                        }
                        height: 35
                    }
                }
            }
        }
    }
}
