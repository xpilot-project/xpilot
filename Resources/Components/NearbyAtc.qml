import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Rectangle {
    id: nearbyAtc
    Layout.preferredWidth: 250
    Layout.fillHeight: true
    Layout.row: 1
    Layout.column: 0
    color: "#272c2e"
    Layout.alignment: Qt.AlignHCenter | Qt.AlignTop

    Text {
        id: title
        color: "#ffffff"
        text: qsTr("Nearby ATC")
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.bold: true
        padding: 5
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        font.family: robotoMono.name
    }

    ScrollView {
        id: scrollView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: title.bottom
        anchors.bottom: parent.bottom
        clip: true
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        anchors.topMargin: 0

        ColumnLayout {
            id: columnLayout
            width: 100
            height: 100

            ListModel {
                id: modelCenter
                ListElement {
                    sector: "LAX_CTR"
                    freq: "125.800"
                }
                ListElement {
                    sector: "OAK_CTR"
                    freq: "132.200"
                }
            }

            ListModel {
                id: modelTower
                ListElement {
                    sector: "LAX_TWR"
                    freq: "120.950"
                }
            }

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
                    model: modelCenter
                    Text {
                        text: sector + " - " + freq
                        font.pixelSize: 14
                        leftPadding: 15
                        padding: 1
                        font.family: robotoMono.name
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
                                        text: sector
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
                    model: modelTower
                    Text {
                        text: sector + " - " + freq
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
                                        text: sector
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
        }
    }
}
