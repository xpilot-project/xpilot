import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import "../Scripts/FrequencyUtils.js" as FrequencyUtils

Rectangle {
    id: nearbyAtc
    Layout.preferredWidth: 250
    Layout.fillHeight: true
    Layout.row: 1
    Layout.column: 0
    color: "#272c2e"
    Layout.alignment: Qt.AlignHCenter | Qt.AlignTop

    property variant enroute;
    property variant approach;
    property variant tower;
    property variant ground;
    property variant delivery;
    property variant atis;
    property variant observers;

    Text {
        id: title
        color: "#ffffff"
        text: qsTr("Nearby ATC")
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        font.family: robotoMono.name
        font.pixelSize: 14
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        padding: 5
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
    }

    ScrollView {
        id: scrollView
        clip: true
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: title.bottom
        anchors.bottom: parent.bottom
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        anchors.topMargin: 0

        ColumnLayout {
            width: 100
            height: 100

            Label {
                id: titleCenter
                color: "#6c757d"
                text: qsTr("Center")
                font.pixelSize: 14
                font.family: robotoMono.name
                renderType: Text.NativeRendering
            }

            Column {
                id: centerControllers
                Repeater {
                    model: enroute
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
                                console.log("requestAtis")
                            }

                            onClicked: {
                                if(mouse.button === Qt.RightButton) {
                                    centerContextMenu.popup()
                                }
                            }
                            onPressAndHold: {
                                if(mouse.source === Qt.MouseEventNotSynthesized) {
                                    centerContextMenu.popup()
                                }
                            }

                            Menu {
                                id: centerContextMenu
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
                                            onClicked: towerContextMenu.close()
                                        }
                                    }
                                    height: 35
                                }
                            }
                        }
                    }
                }
            }

            Label {
                id: titleApproach
                color: "#6c757d"
                text: qsTr("Approach/Departure")
                font.pixelSize: 14
                font.family: robotoMono.name
                renderType: Text.NativeRendering
            }

            Label {
                id: titleTower
                color: "#6c757d"
                text: qsTr("Tower")
                font.pixelSize: 14
                font.family: robotoMono.name
                renderType: Text.NativeRendering
            }

            Column {
                id: towerControllers
                Repeater {
                    model: tower
                    Text {
                        text: `${Callsign} - ${FrequencyUtils.formatNetwork(Frequency)}`
                        font.pixelSize: 14
                        leftPadding: 15
                        padding: 1
                        font.family: robotoMono.name
                        renderType: Text.NativeRendering
                        color: "#ffffff"

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton | Qt.LeftButton
                            cursorShape: Qt.PointingHandCursor

                            onDoubleClicked: {
                                console.log("requestAtis")
                            }

                            onClicked: {
                                if(mouse.button === Qt.RightButton) {
                                    towerContextMenu.popup()
                                }
                            }
                            onPressAndHold: {
                                if(mouse.source === Qt.MouseEventNotSynthesized) {
                                    towerContextMenu.popup()
                                }
                            }

                            Menu {
                                id: towerContextMenu
                                MenuItem {
                                    hoverEnabled: false
                                    enabled: false
                                    contentItem: Text {
                                        text: Callsign
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
                                            onClicked: towerContextMenu.close()
                                        }
                                    }
                                    height: 35
                                }
                            }
                        }
                    }
                }
            }

            Label {
                id: titleGround
                color: "#6c757d"
                text: qsTr("Ground")
                font.pixelSize: 14
                font.family: robotoMono.name
                renderType: Text.NativeRendering
            }

            Label {
                id: titleDelivery
                color: "#6c757d"
                text: qsTr("Delivery")
                font.pixelSize: 14
                font.family: robotoMono.name
                renderType: Text.NativeRendering
            }

            Label {
                id: titleAtis
                color: "#6c757d"
                text: qsTr("ATIS")
                font.pixelSize: 14
                font.family: robotoMono.name
                renderType: Text.NativeRendering
            }

            Label {
                id: titleObservers
                color: "#6c757d"
                text: qsTr("Observers")
                font.pixelSize: 14
                font.family: robotoMono.name
                renderType: Text.NativeRendering
            }
        }
    }
}
