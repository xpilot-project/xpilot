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

    signal startChatSession(string callsign)

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

            // Center

            NearbyAtcGroup {
                internalModel: tower
                groupTitle: "Center"
                onStartChatSession: {
                    console.log("chatSession")
                }
            }

            // Approach/Departure

            NearbyAtcGroup {
                internalModel: approach
                groupTitle: "Approach/Departure"
                onStartChatSession: {
                    console.log("chatSession")
                }
            }

            // Tower

            NearbyAtcGroup {
                internalModel: tower
                groupTitle: "Tower"
                onStartChatSession: {
                    console.log("chatSession")
                }
            }

            // Ground

            NearbyAtcGroup {
                internalModel: ground
                groupTitle: "Ground"
                onStartChatSession: {
                    console.log("chatSession")
                }
            }

            // Delivery

            NearbyAtcGroup {
                internalModel: delivery
                groupTitle: "Delivery"
                onStartChatSession: {
                    console.log("chatSession")
                }
            }

            // ATIS

            NearbyAtcGroup {
                internalModel: atis
                groupTitle: "ATIS"
                onStartChatSession: {
                    console.log("chatSession")
                }
            }

            // Observers

            NearbyAtcGroup {
                internalModel: observers
                groupTitle: "Observers"
                onStartChatSession: {
                    console.log("chatSession")
                }
            }
        }
    }
}
