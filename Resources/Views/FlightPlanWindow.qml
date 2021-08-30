import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import "../Controls"

Window {
    id: flightPlanWindow
    width: 700
    height: 485
    minimumHeight: height
    minimumWidth: width
    maximumHeight: height
    maximumWidth: width
    title: "Flight Plan"
    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    signal closeWindow()
    property string hours: "00"
    property string minutes: "00"
    property string seconds: "00"

    function timeChanged() {
        var date = new Date;
        hours = (date.getUTCHours() < 10 ? '0' : '') + date.getUTCHours();
        minutes = (date.getUTCMinutes() < 10 ? '0' : '') + date.getUTCMinutes();
        seconds = (date.getUTCSeconds() < 10 ? '0' : '') + date.getUTCSeconds();
    }

    // @disable-check M16
    onClosing: {
        closeWindow()
    }

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: flightPlanWindow.timeChanged()
    }

    Item {
        id: header
        x: 0
        y: 0
        width: 700
        height: 50

        Text {
            x: 17
            y: 14
            text: "Flight Plan"
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: false
            leftPadding: 0
            padding: 0
            color: '#0164AD'
            font.pixelSize: 18
            font.family: ubuntuRegular.name
            renderType: Text.NativeRendering
        }

        Rectangle {
            id: headerClip
            clip: true
            anchors.fill: parent
            anchors.rightMargin: 10
            anchors.leftMargin: 10
            anchors.bottomMargin: 5
            anchors.topMargin: 10
            color: "transparent"

            Rectangle {
                anchors.fill: parent
                anchors.topMargin: -20
                anchors.leftMargin: -14
                anchors.rightMargin: -11
                border.width: 1
                border.color: '#DEE2E6'
                color: 'transparent'
            }
        }

        Text {
            id: text1
            x: 105
            y: 18
            color: "#6c757d"
            text: `${hours}:${minutes}:${seconds}`
            renderType: Text.NativeRendering
            font.pixelSize: 14
            verticalAlignment: Text.AlignVCenter
            font.family: ubuntuRegular.name
        }
    }

    GridLayout {
        id: gridLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 0
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        columnSpacing: 10
        rowSpacing: 10
        rows: 7
        columns: 6

        Item {
            id: flightRules
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 0
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text2
                color: "#333333"
                text: qsTr("Flight Rules")
                font.pixelSize: 13
                font.family: ubuntuRegular.name
                renderType: Text.NativeRendering
            }

            CustomComboBox {
                id: clientComboBox
                y: 20
                height: 28
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                model: ["IFR", "VFR", "SVFR"]
            }
        }

        Item {
            id: departure
            Layout.fillWidth: true
            Layout.column: 2
            Layout.row: 0
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text3
                color: "#333333"
                text: qsTr("Departure")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField
                y: 20
                placeholderText: "EGLL"
                onTextChanged: {
                    text = text.toUpperCase()
                }
                validator: RegularExpressionValidator {
                    regularExpression: /[a-zA-Z0-9]{4}/
                }
            }
        }

        Item {
            id: arrival
            Layout.fillWidth: true
            Layout.column: 4
            Layout.row: 0
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text4
                color: "#333333"
                text: qsTr("Arrival")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField1
                y: 20
                placeholderText: "KSFO"
                onTextChanged: {
                    text = text.toUpperCase()
                }
                validator: RegularExpressionValidator {
                    regularExpression: /[a-zA-Z0-9]{4}/
                }
            }
        }

        Item {
            id: alternate
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 1
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text5
                color: "#333333"
                text: qsTr("Alternate")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField2
                y: 20
                placeholderText: "KLAX"
                onTextChanged: {
                    text = text.toUpperCase()
                }
                validator: RegularExpressionValidator {
                    regularExpression: /[a-zA-Z0-9]{4}/
                }
            }
        }

        Item {
            id: offBlock
            Layout.fillWidth: true
            Layout.column: 2
            Layout.row: 1
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text6
                color: "#333333"
                text: qsTr("Off Block (UTC)")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField3
                y: 20
                placeholderText: "1650"
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{4}/
                }
            }
        }

        Item {
            id: timeEnroute
            Layout.fillWidth: true
            Layout.column: 4
            Layout.row: 1
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text7
                color: "#333333"
                text: qsTr("Time Enroute")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField10
                y: 20
                width: 105
                height: 30
                anchors.right: parent.right
                placeholderText: "hh"
                anchors.rightMargin: 120
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{2}/
                }
            }

            CustomTextField {
                id: clientTextField11
                y: 20
                height: 30
                anchors.left: parent.left
                placeholderText: "mm"
                anchors.leftMargin: 120
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{2}/
                }
            }
        }

        Item {
            id: airspeed
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 2
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text8
                color: "#333333"
                text: qsTr("Airspeed")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField6
                y: 20
                placeholderText: "494"
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{4}/
                }
            }
        }

        Item {
            id: altitude
            Layout.fillWidth: true
            Layout.column: 2
            Layout.row: 2
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text9
                color: "#333333"
                text: qsTr("Altitude (ft)")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField7
                y: 20
                placeholderText: "38000"
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{5}/
                }
            }
        }

        Item {
            id: fuelAvailable
            Layout.fillWidth: true
            Layout.column: 4
            Layout.row: 2
            Layout.columnSpan: 2
            Layout.preferredHeight: 50
            Layout.preferredWidth: 50

            Text {
                id: text10
                color: "#333333"
                text: qsTr("Fuel Available")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            CustomTextField {
                id: clientTextField8
                y: 20
                width: 105
                height: 30
                anchors.right: parent.right
                placeholderText: "hh"
                anchors.rightMargin: 120
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{2}/
                }
            }

            CustomTextField {
                id: clientTextField9
                y: 20
                height: 30
                anchors.left: parent.left
                placeholderText: "mm"
                anchors.leftMargin: 120
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{2}/
                }
            }
        }

        Item {
            id: route
            Text {
                id: text11
                color: "#333333"
                text: qsTr("Route")
                renderType: Text.NativeRendering
                font.pixelSize: 13
                font.family: ubuntuRegular.name
            }

            TextArea {
                id: textArea1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: text11.bottom
                anchors.bottom: parent.bottom
                font.pixelSize: 13
                font.capitalization: Font.AllUppercase
                leftPadding: 5
                font.family: ubuntuRegular.name
                selectionColor: "#0164AD"
                selectedTextColor: "#ffffff"
                padding: 5
                anchors.topMargin: 5
                renderType: Text.NativeRendering
                background: Rectangle {
                    border.color: textArea1.focus ? '#565E64' : '#babfc4'
                }
            }
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.rowSpan: 2
            Layout.fillWidth: true
            Layout.row: 3
            Layout.preferredWidth: 50
            Layout.preferredHeight: 100
        }

        Item {
            id: remarks
            Text {
                id: text12
                color: "#333333"
                text: qsTr("Remarks")
                font.pixelSize: 13
                renderType: Text.NativeRendering
                font.family: ubuntuRegular.name
            }

            TextArea {
                id: textArea
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: text12.bottom
                anchors.bottom: parent.bottom
                font.pixelSize: 13
                leftPadding: 5
                padding: 5
                font.family: ubuntuRegular.name
                font.capitalization: Font.AllUppercase
                anchors.topMargin: 5
                selectionColor: "#0164AD"
                selectedTextColor: "#ffffff"
                renderType: Text.NativeRendering
                background: Rectangle {
                    border.color: textArea.focus ? '#565E64' : '#babfc4'
                }
            }
            Layout.column: 3
            Layout.columnSpan: 3
            Layout.rowSpan: 2
            Layout.fillWidth: true
            Layout.row: 3
            Layout.preferredWidth: 50
            Layout.preferredHeight: 100
        }

        Item {
            id: equipment
            Text {
                id: text13
                color: "#333333"
                text: qsTr("Equipment")
                renderType: Text.NativeRendering
                font.pixelSize: 13
                font.family: ubuntuRegular.name
            }

            CustomComboBox {
                id: clientComboBox3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: text13.bottom
                anchors.rightMargin: 0
                anchors.topMargin: 5
                anchors.leftMargin: 0
                model: ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"]
            }
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.row: 5
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
        }

        Item {
            id: voiceCapability
            Text {
                id: text14
                color: "#333333"
                text: qsTr("Voice Capability")
                renderType: Text.NativeRendering
                font.pixelSize: 13
                font.family: ubuntuRegular.name
            }

            CustomComboBox {
                id: clientComboBox2
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: text14.bottom
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                anchors.topMargin: 5
                textRole: "text"
                valueRole: "value"
                model: [
                    { value: "V", text: "Send and Receive Voice" },
                    { value: "R", text: "Receive Only" },
                    { value: "T", text: "Text Only" }
                ]
            }
            Layout.column: 3
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.row: 5
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
        }

        Item {
            id: sendFlightPlan
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.preferredWidth: 50
            Layout.fillWidth: true
            Layout.row: 6
            Layout.preferredHeight: 50

            BlueButton {
                id: btnSendFlightPlan
                height: 35
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 14
                font.family: ubuntuRegular.name
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                text: "Send Flight Plan"
            }
        }


        Item {
            id: fetchFromServer
            Layout.column: 2
            Layout.columnSpan: 2
            Layout.preferredWidth: 50
            Layout.fillWidth: true
            Layout.row: 6
            Layout.preferredHeight: 50

            BlueButton {
                id: btnFetchFlightPlan
                height: 35
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 14
                font.family: ubuntuRegular.name
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                text: "Fetch from Server"
            }
        }

        Item {
            id: clearFlightPlan
            Layout.column: 4
            Layout.columnSpan: 2
            Layout.preferredWidth: 50
            Layout.fillWidth: true
            Layout.row: 6
            Layout.preferredHeight: 50

            BlueButton {
                id: btnClearFlightPlan
                height: 35
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 14
                font.family: ubuntuRegular.name
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                text: "Clear Flight Plan"
            }
        }
    }
}
