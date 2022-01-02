import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.2
import "../Controls"
import "../Components"

Window {
    property QtObject importIcaoFpl

    id: flightPlanWindow
    width: 760
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
    property var networkConnected: false

    function timeChanged() {
        var date = new Date;
        hours = (date.getUTCHours() < 10 ? '0' : '') + date.getUTCHours();
        minutes = (date.getUTCMinutes() < 10 ? '0' : '') + date.getUTCMinutes();
        seconds = (date.getUTCSeconds() < 10 ? '0' : '') + date.getUTCSeconds();
    }

    function blankIfZero(val) {
        return Math.abs(val) > 0 ? val : ""
    }

    // @disable-check M16
    onClosing: {
        closeWindow()
    }

    Component.onCompleted: {
        networkManager.loadLastFlightPlan()
    }

    Connections {
        target: importIcaoFpl
        function onImportFlightPlan(flightPlan) {
            parseIcaoFpl(flightPlan)
            importIcaoFpl.destroy()
        }
    }

    Connections {
        target: networkManager

        function onLastFlightPlanReceived(flightPlan) {
            populateFlightPlan(flightPlan)
        }

        function onRemoteFlightPlanReceived(flightPlan) {
            populateFlightPlan(flightPlan)
        }

        function onNoFlightPlanFound() {
            errorDialog.errorText = "No flight plan found on the server."
            errorDialog.open()
        }
    }

    ErrorDialog {
        id: errorDialog
    }

    function populateFlightPlan(flightPlan) {
        clearFlightPlan()
        var fp = JSON.parse(flightPlan)
        txtDeparture.text = fp.departure
        txtArrival.text = fp.arrival
        txtAlternate.text = fp.alternate
        txtOffBlock.text = blankIfZero(fp.off_block)
        txtEnrouteHours.text = blankIfZero(fp.enroute_hours)
        txtEnrouteMinutes.text = blankIfZero(fp.enroute_minutes)
        txtFuelHours.text = blankIfZero(fp.fuel_hours)
        txtFuelMinutes.text = blankIfZero(fp.fuel_minutes)
        txtRoute.text = fp.route
        txtRemarks.text = fp.remarks
        txtAltitude.text = blankIfZero(fp.altitude)
        txtAirspeed.text = blankIfZero(fp.airspeed)
        var idx = voiceCapability.indexOfValue(fp.voice_capability)
        voiceCapability.currentIndex = idx
        var equipmentSlashIdx = fp.equipment.lastIndexOf('/')
        equipmentCombo.currentIndex = equipmentCombo.indexOfValue(fp.equipment.substring(equipmentSlashIdx + 1))
        switch(fp.flight_rules) {
        case "I":
        case "IFR":
            flightRulesCombo.currentIndex = 0
            break;
        case "V":
        case "VFR":
            flightRulesCombo.currentIndex = 1
            break;
        case "S":
        case "SVFR":
            flightRulesCombo.currentIndex = 2
            break;
        }
    }

    function clearFlightPlan() {
        flightRulesCombo.currentIndex = -1
        txtDeparture.text = ""
        txtArrival.text = ""
        txtAlternate.text = ""
        txtOffBlock.text = ""
        txtEnrouteHours.text = ""
        txtEnrouteMinutes.text = ""
        txtRoute.text = ""
        txtRemarks.text = ""
        txtAirspeed.text = ""
        txtAltitude.text = ""
        equipmentCombo.currentIndex = -1
        voiceCapability.currentIndex = -1
    }

    function parseIcaoFpl(fp) {
        var patt = new RegExp(/\(FPL-([A-Z0-9]{1,7})-([IVYZ])([SNGMX])[\r\n]*-([A-Z0-9]{2,4})\/([JHML])\-([A-Z0-9]+)\/([A-Z0-9]{1,5})[\r\n]*-([A-Z0-9]{4})([0-9]{4})[\r\n]*-([KNM])([0-9]{3,4})F([0-9]{3})((.|\n)*)-([A-Z0-9]{4})([\d]{2})([\d]{2})\s?([A-Z]{4})?[\r\n]*-?((.|\n|\r)*)\)/)
        var match = fp.match(patt)
        if(match) {
            clearFlightPlan();

            flightRulesCombo.currentIndex = (match[2] === "I" || match[2] === "Y") ? 0 : 1
            txtDeparture.text = match[8].toUpperCase().trim()
            txtArrival.text = match[15].toUpperCase().trim()
            txtAlternate.text = match[18].toUpperCase().trim()
            txtOffBlock.text = match[9].trim()
            txtEnrouteHours.text = parseInt(match[16]) > 0 ? parseInt(match[16]).toString() : ""
            txtEnrouteMinutes.text = parseInt(match[17]) > 0 ? parseInt(match[17]).toString() : ""
            txtRoute.text = match[13].replace(/\s+/g, ' ').toUpperCase().trim()
            txtRemarks.text = match[19].replace(/\s+/g, ' ').toUpperCase().trim()

            if(match[10] === "K") {
                // km/h to knots
                txtAirspeed.text = (parseInt(match[11]) * 0.539957).toString()
            }
            else if(match[10] === "M") {
                // mach to knots
                txtAirspeed.text = Math.round(((parseInt(match[11]) / 100) * 666.739)).toString()
            }
            else if(match[10] === "N") {
                // knots true airspeed
                txtAirspeed.text = (parseInt(match[11])).toString()
            }

            txtAltitude.text = (parseInt(match[12]) * 100).toString()

            var field10a = match[6]
            var field10b = match[7]

            icaoToFaa(field10a, field10b)
        }
    }

    function icaoToFaa(field10a, field10b) {
        var w = field10a.match('W')
        var g = field10a.match('G');
        var rcix = field10a.match('R|C|I|X');
        var d = field10a.match('D');
        var t = field10a.match('T');
        var none = !(w || g || rcix || d || t);

        var cpsehl = field10b.match('C|P|S|E|H|L');
        var axi = field10b.match('A|X|I');
        var n = field10b.match('N');

        var idx;

        if (g && w && cpsehl) {
            idx = equipmentCombo.indexOfValue('L')
            equipmentCombo.currentIndex = idx
        }
        else if (rcix && w && cpsehl) {
            idx = equipmentCombo.indexOfValue('Z')
            equipmentCombo.currentIndex = idx
        }
        else if (w && cpsehl) {
            idx = equipmentCombo.indexOfValue('W')
            equipmentCombo.currentIndex = idx
        }
        else if (g && cpsehl) {
            idx = equipmentCombo.indexOfValue('G')
            equipmentCombo.currentIndex = idx
        }
        else if (g && axi) {
            idx = equipmentCombo.indexOfValue('S')
            equipmentCombo.currentIndex = idx
        }
        else if (g && n) {
            idx = equipmentCombo.indexOfValue('V')
            equipmentCombo.currentIndex = idx
        }
        else if (rcix && cpsehl) {
            idx = equipmentCombo.indexOfValue('I')
            equipmentCombo.currentIndex = idx
        }
        else if (rcix && axi) {
            idx = equipmentCombo.indexOfValue('C')
            equipmentCombo.currentIndex = idx
        }
        else if (rcix && n) {
            idx = equipmentCombo.indexOfValue('Y')
            equipmentCombo.currentIndex = idx
        }
        else if (d && cpsehl) {
            idx = equipmentCombo.indexOfValue('A')
            equipmentCombo.currentIndex = idx
        }
        else if (d && axi) {
            idx = equipmentCombo.indexOfValue('B')
            equipmentCombo.currentIndex = idx
        }
        else if (d && n) {
            idx = equipmentCombo.indexOfValue('D')
            equipmentCombo.currentIndex = idx
        }
        else if (t && cpsehl) {
            idx = equipmentCombo.indexOfValue('P')
            equipmentCombo.currentIndex = idx
        }
        else if (t && axi) {
            idx = equipmentCombo.indexOfValue('N')
            equipmentCombo.currentIndex = idx
        }
        else if (t && n) {
            idx = equipmentCombo.indexOfValue('M')
            equipmentCombo.currentIndex = idx
        }
        else if (none && cpsehl) {
            idx = equipmentCombo.indexOfValue('U')
            equipmentCombo.currentIndex = idx
        }
        else if (none && axi) {
            idx = equipmentCombo.indexOfValue('T')
            equipmentCombo.currentIndex = idx
        }
        else if (none && n) {
            idx = equipmentCombo.indexOfValue('X')
            equipmentCombo.currentIndex = idx
        }
        else {
            idx = equipmentCombo.indexOfValue('X')
            equipmentCombo.currentIndex = idx
        }
    }

    function serializeFlightPlan() {
        var json = {}
        json["flight_rules"] = flightRulesCombo.currentValue
        json["departure"] = txtDeparture.text.toUpperCase()
        json["arrival"] = txtArrival.text.toUpperCase()
        json["alternate"] = txtAlternate.text.toUpperCase()
        json["off_block"] = txtOffBlock.text
        json["enroute_hours"] = txtEnrouteHours.text
        json["enroute_minutes"] = txtEnrouteMinutes.text
        json["airspeed"] = txtAirspeed.text

        var alt = parseInt(txtAltitude.text)
        if(alt < 1000) alt *= 100
        json["altitude"] = alt

        json["fuel_hours"] = txtFuelHours.text
        json["fuel_minutes"] = txtFuelMinutes.text
        json["route"] = txtRoute.text.toUpperCase()
        json["remarks"] = txtRemarks.text.toUpperCase()
        json["equipment_suffix"] = equipmentCombo.currentValue
        json["voice_capability"] = voiceCapability.currentValue
        return JSON.stringify(json)
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
        width: 760
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
        }
    }

    GrayButton {
        id: btnFileOnline
        font.pixelSize: 14
        text: "File Online"
        anchors.right: btnImportIcaoFp.left
        anchors.rightMargin: 10

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                Qt.openUrlExternally("https://my.vatsim.net/pilots/flightplan")
            }
        }
    }

    GrayButton {
        id: btnImportIcaoFp
        font.pixelSize: 14
        text: "Import ICAO FPL"
        anchors.right: parent.right
        anchors.rightMargin: 15

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                var comp = Qt.createComponent("qrc:/Resources/Views/ImportIcaoFpl.qml")
                if(comp.status === Component.Ready) {
                    importIcaoFpl = comp.createObject(flightPlanWindow)
                    importIcaoFpl.show()
                }
            }
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
                renderType: Text.NativeRendering
            }

            CustomComboBox {
                id: flightRulesCombo
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
            }

            CustomTextField {
                id: txtDeparture
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
            }

            CustomTextField {
                id: txtArrival
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
                color: "#333333"
                text: qsTr("Alternate")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            CustomTextField {
                id: txtAlternate
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
                color: "#333333"
                text: qsTr("Off Block (UTC)")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            CustomTextField {
                id: txtOffBlock
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
                color: "#333333"
                text: qsTr("Time Enroute")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            CustomTextField {
                id: txtEnrouteHours
                y: 20
                width: 105
                height: 30
                anchors.right: parent.right
                placeholderText: "hh"
                anchors.rightMargin: 130
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{2}/
                }
            }

            CustomTextField {
                id: txtEnrouteMinutes
                y: 20
                height: 30
                anchors.left: parent.left
                placeholderText: "mm"
                anchors.leftMargin: 130
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
                color: "#333333"
                text: qsTr("Airspeed")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            CustomTextField {
                id: txtAirspeed
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
                color: "#333333"
                text: qsTr("Altitude (ft)")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            CustomTextField {
                id: txtAltitude
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
                color: "#333333"
                text: qsTr("Fuel Available")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            CustomTextField {
                id: txtFuelHours
                y: 20
                width: 105
                height: 30
                anchors.right: parent.right
                placeholderText: "hh"
                anchors.rightMargin: 130
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{2}/
                }
            }

            CustomTextField {
                id: txtFuelMinutes
                y: 20
                height: 30
                anchors.left: parent.left
                placeholderText: "mm"
                anchors.leftMargin: 130
                validator: RegularExpressionValidator {
                    regularExpression: /[0-9]{2}/
                }
            }
        }

        Item {
            id: route
            Text {
                id: routeLabel
                color: "#333333"
                text: qsTr("Route")
                renderType: Text.NativeRendering
                font.pixelSize: 13
            }


            Flickable {
                anchors.fill: parent
                anchors.top: routeLabel.bottom
                anchors.topMargin: 20

                ScrollBar.vertical: ScrollBar { }

                TextArea.flickable: TextArea {
                    id: txtRoute
                    Layout.column: 0
                    Layout.row: 0
                    background: Rectangle {
                        border.color: txtRoute.focus ? '#565E64' : '#babfc4'
                    }
                    font.pixelSize: 13
                    color: '#333333'
                    selectionColor: "#0164AD"
                    selectedTextColor: "#ffffff"
                    selectByMouse: true
                    renderType: Text.NativeRendering
                    wrapMode: TextEdit.WordWrap
                    font.capitalization: Font.AllUppercase
                    leftPadding: 5
                    Keys.onTabPressed: {
                        event.accepted = true
                    }
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
                id: remarksLabel
                color: "#333333"
                text: qsTr("Remarks")
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            Flickable {
                anchors.fill: parent
                anchors.top: remarksLabel.bottom
                anchors.topMargin: 20

                ScrollBar.vertical: ScrollBar { }

                TextArea.flickable: TextArea {
                    id: txtRemarks
                    Layout.column: 0
                    Layout.row: 0
                    background: Rectangle {
                        border.color: txtRemarks.focus ? '#565E64' : '#babfc4'
                    }
                    font.pixelSize: 13
                    color: '#333333'
                    selectionColor: "#0164AD"
                    selectedTextColor: "#ffffff"
                    selectByMouse: true
                    renderType: Text.NativeRendering
                    wrapMode: TextEdit.WordWrap
                    font.capitalization: Font.AllUppercase
                    leftPadding: 5
                    Keys.onTabPressed: {
                        event.accepted = true
                    }
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
                id: equipmentLabel
                color: "#333333"
                text: qsTr("Navigational Equipment Suffix")
                renderType: Text.NativeRendering
                font.pixelSize: 13
            }

            CustomComboBox {
                id: equipmentCombo
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: equipmentLabel.bottom
                anchors.rightMargin: 0
                anchors.topMargin: 5
                anchors.leftMargin: 0
                textRole: "text"
                valueRole: "value"
                model: [
                    {value: "X", text: "/X - No Transponder (No DME)"},
                    {value: "T", text: "/T - Transponder with no Mode C (No DME)"},
                    {value: "U", text: "/U - Transponder with Mode C (No DME)"},
                    {value: "D", text: "/D - No Transponder (DME)"},
                    {value: "B", text: "/B - Transponder with no Mode C (DME)"},
                    {value: "A", text: "/A - Transponder with Mode C (DME)"},
                    {value: "M", text: "/M - No Transponder (TACAN)"},
                    {value: "N", text: "/N - Transponder with no Mode C (TACAN)"},
                    {value: "P", text: "/P - Transponder with Mode C (TACAN)"},
                    {value: "Y", text: "/Y - RNAV, No GNSS, No Transponder"},
                    {value: "C", text: "/C - RNAV, No GNSS, Transponder with No Mode C"},
                    {value: "I", text: "/I - RNAV, No GNSS, Transponder with Mode C"},
                    {value: "V", text: "/V - GNSS, No Transponder"},
                    {value: "S", text: "/S - GNSS, Transponder with No Mode C"},
                    {value: "G", text: "/G - GNSS, Transponder with Mode C"},
                    {value: "W", text: "/W - No GNSS, No RNAV, Transponder with Mode C (RVSM)"},
                    {value: "Z", text: "/Z - RNAV, No GNSS, Transponder with Mode C (RVSM)"},
                    {value: "L", text: "/L - GNSS, Transponder with Mode C (RVSM)"},
                ]
            }
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.row: 5
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
        }

        Item {
            Layout.column: 3
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.row: 5
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50

            Text {
                id: text14
                color: "#333333"
                text: qsTr("Voice Capability")
                renderType: Text.NativeRendering
                font.pixelSize: 13
            }

            CustomComboBox {
                id: voiceCapability
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: text14.bottom
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                anchors.topMargin: 5
                textRole: "text"
                valueRole: "value"
                model: [
                    { value: "V", text: "Transmit and Receive Voice" },
                    { value: "R", text: "Receive Only" },
                    { value: "T", text: "Text Only" }
                ]
            }
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
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                text: "Send Flight Plan"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if(!networkConnected) {
                            errorDialog.errorText = "Not connected to the network."
                            errorDialog.open()
                        }
                        else if(voiceCapability.currentIndex == -1) {
                            errorDialog.errorText = "Please indicate your voice capability."
                            errorDialog.open()
                        }
                        else if(equipmentCombo.currentIndex == -1) {
                            errorDialog.errorText = "Please indicate your navigational equipment suffix."
                            errorDialog.open()
                        }
                        else {
                            var fp = serializeFlightPlan()
                            networkManager.sendFlightPlan(fp)
                            close()
                        }
                    }
                }
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

            GrayButton {
                id: btnFetchFlightPlan
                height: 35
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 14
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                text: "Fetch from Server"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        networkManager.requestFlightPlan()
                    }
                }
            }
        }

        Item {
            Layout.column: 4
            Layout.columnSpan: 2
            Layout.preferredWidth: 50
            Layout.fillWidth: true
            Layout.row: 6
            Layout.preferredHeight: 50

            GrayButton {
                id: btnClearFlightPlan
                height: 35
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 14
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                text: "Clear Flight Plan"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearFlightPlan()
                    }
                }
            }
        }
    }
}
