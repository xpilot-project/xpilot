import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import "../Scripts/FrequencyUtils.js" as FrequencyUtils

Column {
    id: root

    property variant internalModel
    property string groupTitle

    signal sendPrivateMessage(string callsign)

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
            text: `${Callsign} - ${FrequencyUtils.printFrequency(Frequency)}`
            font.pixelSize: 14
            font.family: robotoMono.name
            leftPadding: 15
            color: "#ffffff"
            renderType: Text.NativeRendering

            ToolTip.visible: callsignMouseArea.containsMouse
            ToolTip.text: RealName

            MouseArea {
                id: callsignMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.RightButton | Qt.LeftButton
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

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
                            text: Callsign
                            font.pixelSize: 16
                            font.family: robotoMono.name
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
                                onClicked: {
                                    sendPrivateMessage(Callsign)
                                    ctxtMenu.close()
                                }
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
                                onClicked: {
                                    networkManager.requestControllerAtis(Callsign)
                                    ctxtMenu.close()
                                }
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
                                onClicked: {
                                    var freq = FrequencyUtils.normalize25KhzFrequency(Frequency)
                                    xplaneAdapter.setCom1Frequency(freq)
                                    ctxtMenu.close()
                                }
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
                                onClicked: {
                                    var freq = FrequencyUtils.normalize25KhzFrequency(Frequency)
                                    xplaneAdapter.setCom2Frequency(freq)
                                    ctxtMenu.close()
                                }
                            }
                        }
                        height: 35
                    }
                }
            }
        }
    }
}
