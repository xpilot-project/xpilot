import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic

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
        renderType: Text.NativeRendering
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
                internalModel: enroute
                groupTitle: "Center"
                onSendPrivateMessage: (callsign) => {
                    startChatSession(callsign)
                }
            }

            // Approach/Departure
            NearbyAtcGroup {
                internalModel: approach
                groupTitle: "Approach/Departure"
                onSendPrivateMessage: (callsign) => {
                    startChatSession(callsign)
                }
            }

            // Tower
            NearbyAtcGroup {
                internalModel: tower
                groupTitle: "Tower"
                onSendPrivateMessage: (callsign) => {
                    startChatSession(callsign)
                }
            }

            // Ground
            NearbyAtcGroup {
                internalModel: ground
                groupTitle: "Ground"
                onSendPrivateMessage: (callsign) => {
                    startChatSession(callsign)
                }
            }

            // Delivery
            NearbyAtcGroup {
                internalModel: delivery
                groupTitle: "Delivery"
                onSendPrivateMessage: (callsign) => {
                    startChatSession(callsign)
                }
            }

            // ATIS
            NearbyAtcGroup {
                internalModel: atis
                groupTitle: "ATIS"
                onSendPrivateMessage: (callsign) => {
                    startChatSession(callsign)
                }
            }
        }
    }
}
