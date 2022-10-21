import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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

                onClicked: (mouse) => {
                    if(mouse.button === Qt.RightButton) {
                        ctxtMenu.popup()
                    }
                }
                onPressAndHold: (mouse) => {
                    if(mouse.source === Qt.MouseEventNotSynthesized) {
                        ctxtMenu.popup()
                    }
                }

                Menu {
                    id: ctxtMenu

                    background: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 40
                        border.color: "black"
                    }

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
                        id: ctrlSendPrivateMessage
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
                        background: Item {
                            implicitWidth: 200
                            implicitHeight: 40
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 1
                                color: ctrlSendPrivateMessage.highlighted ? "#ADB5BD" : "transparent"
                            }
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                        height: 35
                    }
                    MenuItem {
                        id: ctrlRequestControllerInfo
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
                        background: Item {
                            implicitWidth: 200
                            implicitHeight: 40
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 1
                                color: ctrlRequestControllerInfo.highlighted ? "#ADB5BD" : "transparent"
                            }
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                        height: 35
                    }
                    MenuItem {
                        id: ctrlTuneCom1
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
                        background: Item {
                            implicitWidth: 200
                            implicitHeight: 40
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 1
                                color: ctrlTuneCom1.highlighted ? "#ADB5BD" : "transparent"
                            }
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                        height: 35
                    }
                    MenuItem {
                        id: ctrlTuneCom2
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
                        background: Item {
                            implicitWidth: 200
                            implicitHeight: 40
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 1
                                color: ctrlTuneCom2.highlighted ? "#ADB5BD" : "transparent"
                            }
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                        height: 35
                    }
                }
            }
        }
    }
}
