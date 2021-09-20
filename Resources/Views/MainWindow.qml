import QtQuick 2.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.12
import QtQuick.Window 2.15
import QtQuick.Layouts 1.12
import QtWebSockets 1.2
import "../Scripts/FrequencyUtils.js" as FrequencyUtils
import "../Scripts/TimestampUtils.js" as TimestampUtils
import "../Components"
import "../Controls"

Window {
    id: mainWindow
    title: "xPilot"
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    width: 800
    height: 250
    minimumHeight: 250
    minimumWidth: 800
    color: "#272C2E"

    property QtObject connectWindow
    property QtObject settingsWindow
    property QtObject flightPlanWindow
    property int currentTab
    property bool simConnected: false

    signal setTransponderCode(int code);
    signal setTransponderModeC(bool active);
    signal setTransponderIdent();
    signal setRadioStack(int radio, int frequency);
    signal requestConfig();
    signal updateConfig(var config);

    FontLoader {
        id: ubuntuRegular
        source: "../Fonts/Ubuntu-Regular.ttf"
    }

    FontLoader {
        id: robotoMono
        source: "../Fonts/Roboto-Mono.ttf"
    }

    Component.onCompleted: {
        appendInfoMessage("Waiting for X-Plane connection... Please make sure X-Plane is running and a flight is loaded.");
    }

    Connections {
        target: connectWindow
        function onCloseWindow() {
            connectWindow.destroy()
        }
    }

    Connections {
        target: flightPlanWindow
        function onCloseWindow() {
            flightPlanWindow.destroy()
        }
    }

    Connections {
        target: settingsWindow
        function onCloseWindow() {
            settingsWindow.destroy()
        }
        function onRequestConfig() {
            requestConfig()
        }
        function onUpdateConfig(config) {
            updateConfig(config)
        }
    }

    Connections {
        target: ipc

        function onNotificationPosted(type, message) {
            switch(type) {
            case 0: // info
                appendInfoMessage(message)
                break;
            case 1: // warning
                appendWarningMessage(message)
                break;
            case 2: // error
                appendErrorMessage(message)
                break;
            }
        }

        function onSimulatorConnected(isConnected) {
            if(isConnected) {
                if(!simConnected) {
                    appendInfoMessage("X-Plane connection established.")
                }
                simConnected = true
            } else {
                if(simConnected) {
                    appendErrorMessage("X-Plane connection lost.")
                }
                simConnected = false
            }

            toolbar.simConnected = isConnected
        }

        function onRadioStackReceived(stack)
        {
            radioStack.avionicsPower = stack.avionicsPowerOn;
            radioStack.com1Frequency = FrequencyUtils.printFrequency(stack.com1Frequency);
            radioStack.com2Frequency = FrequencyUtils.printFrequency(stack.com2Frequency);
            radioStack.isCom1RxEnabled = stack.com1ReceiveEnabled;
            radioStack.isCom2RxEnabled = stack.com2ReceiveEnabled;
            radioStack.isCom1TxEnabled = stack.transmitComSelection === 6;
            radioStack.isCom2TxEnabled = stack.transmitComSelection === 7;
        }

        function onNearbyAtcReceived(stations) {
            stations.forEach(function(station) {
                if(station.callsign.endsWith("_CTR") || station.callsign.endsWith("_FSS")) {
                    nearbyEnroute.append(station)
                }
                else if(station.callsign.endsWith("_APP") || station.callsign.endsWith("_DEP")) {
                    nearbyApproach.append(station)
                }
                else if(station.callsign.endsWith("_TWR")) {
                    nearbyTower.append(station)
                }
                else if(station.callsign.endsWith("_GND")) {
                    nearbyGround.append(station)
                }
                else if(station.callsign.endsWith("_DEL")) {
                    nearbyDelivery.append(station)
                }
                else if(station.callsign.endsWith("_ATIS")) {
                    nearbyAtis.append(station)
                }
                else {
                    nearbyObservers.append(station)
                }
            })
        }
    }

    //    function appendMessage(tabId, message) {
    //        var element = cliModel.get(tabId + 1);
    //        if(element) {
    //            element.attributes.append({timestamp:currentTimestamp(),message:message});
    //        } else {
    //            cliModel.append({tabId: tabId, attributes: []});
    //            appendMessage(tabId, message);
    //        }
    //    }

    function appendMessage(message) {
        var model = cliModel.get(0)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`})
    }

    function appendInfoMessage(message) {
        var model = cliModel.get(0)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`, msgColor:"#F1C40F"}) // yellow
    }

    function appendErrorMessage(message) {
        var model = cliModel.get(0)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`, msgColor:"#EB2F06"}) // red
    }

    function appendWarningMessage(message) {
        var model = cliModel.get(0)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`, msgColor:"#E67E22"}) // orange
    }

    function appendNote(message) {
        var model = cliModel.get(1)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`})
    }

    function clearNotes() {
        var model = cliModel.get(1)
        model.attributes.clear()
    }

    function clearMessages() {
        var model = cliModel.get(0)
        model.attributes.clear()
    }

    Rectangle {
        id: windowFrame
        anchors.fill: parent
        color: "transparent"
        border.color: Qt.platform.os === 'osx' ? 'transparent' : '#000000'
        z: 200
    }

    GridLayout {
        anchors.fill: parent
        rowSpacing: 0
        columnSpacing: 0
        rows: 2
        columns: 2
        z: 100

        Rectangle {
            id: radioStackContainer
            Layout.preferredWidth: 250
            Layout.preferredHeight: 75
            Layout.row: 0
            Layout.column: 0
            color: "#0164AD"

            RadioStack {
                id: radioStack
                anchors.top: parent.top
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 0
            }
        }

        Rectangle {
            id: toolbarContainer
            Layout.row: 0
            Layout.column: 1
            color: "transparent"
            Layout.preferredHeight: 75
            Layout.fillWidth: true

            Toolbar {
                id: toolbar

                onToggleModeC: {
                    setTransponderModeC(active)
                }

                onSquawkIdent: {
                    setTransponderIdent()
                }
            }
        }

        Rectangle {
            id: nearbyAtcContainer
            Layout.row: 1
            Layout.column: 0
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            color: "transparent"

            ListModel {
                id: nearbyEnroute
            }

            ListModel {
                id: nearbyApproach
            }

            ListModel {
                id: nearbyTower
            }

            ListModel {
                id: nearbyGround
            }

            ListModel {
                id: nearbyDelivery
            }

            ListModel {
                id: nearbyAtis
            }

            ListModel {
                id: nearbyObservers
            }

            NearbyAtc {
                id: nearbyAtc
                anchors.fill: parent
                enroute: nearbyEnroute
                approach: nearbyApproach
                tower: nearbyTower
                ground: nearbyGround
                delivery: nearbyDelivery
                atis: nearbyAtis
                observers: nearbyObservers
            }
        }

        Rectangle {
            id: msgControl
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 1
            color: "#141618"

            Rectangle {
                anchors.fill: parent
                anchors.margins: 10
                anchors.topMargin: 39
                border.color: "#5c5c5c"
                border.width: 1
                color: "transparent"
            }

            ListModel {
                id: tabModel
                ListElement {
                    title: "Messages"
                    disposable: false
                }
                ListElement {
                    title: "Notes"
                    disposable: false
                }
            }

            Component {
                id: tabDelegate
                Item {
                    id: tab

                    property color frameColor: "#5C5C5C"
                    property color fillColor: "#141618"

                    property var view: ListView.view
                    property int itemIndex: index

                    implicitWidth: disposable ? text.width + 45 : text.width + 20
                    implicitHeight: 30

                    Rectangle {
                        id: topRect
                        anchors.fill: parent
                        radius: 8
                        color: fillColor
                        border.width: 1
                        border.color: frameColor
                    }

                    Rectangle {
                        id: bottomRect
                        anchors.bottom: parent.bottom
                        anchors.left: topRect.left
                        anchors.right: topRect.right
                        height: 1 / 2 * parent.height
                        color: fillColor
                        border.width: 1
                        border.color: frameColor
                    }

                    // remove weird line that runs through the middle of the tab
                    Rectangle {
                        anchors {
                            fill: bottomRect
                            leftMargin: bottomRect.border.width
                            bottomMargin: bottomRect.border.width
                            rightMargin: bottomRect.border.width
                        }
                        color: fillColor
                    }

                    // hides bottom border on active tab
                    Rectangle {
                        visible: itemIndex === view.currentIndex
                        width: tab.width - 2
                        height: 2
                        color: fillColor
                        y: parent.height - 2
                        x: (tab.width - tab.width) + 1
                    }

                    Text {
                        id: text
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 10
                        text: title
                        color: itemIndex === view.currentIndex ? "white" : frameColor
                        font.family: robotoMono.name
                        font.pixelSize: 13
                    }

                    WindowControlButton {
                        id: btnClose
                        visible: disposable
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 0
                        z: 100

                        icon.source: "../Icons/CloseIcon.svg"
                        icon.color: "transparent"
                        icon.width: 18
                        icon.height: 18
                        onHoveredChanged: hovered ? icon.color = "white" : icon.color = "transparent"

                        MouseArea {
                            anchors.fill: btnClose
                            cursorShape: Qt.PointingHandCursor
                            onClicked: tabModel.remove(itemIndex)
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onClicked: {
                            currentTab = itemIndex
                            view.currentIndex = itemIndex
                        }
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }

            ListView {
                model: tabModel
                delegate: tabDelegate
                anchors.fill: parent
                anchors.margins: 10
                orientation: ListView.Horizontal
                spacing: -1
                clip: true
            }

            ListModel {
                id: cliModel
                ListElement {
                    tabId: 0 // messages
                    attributes: []
                }
                ListElement {
                    tabId: 1 // notes
                    attributes: []
                }
            }

            Component {
                id: cliDelegate

                Rectangle {
                    id: messages
                    color: 'transparent'
                    anchors.fill: parent
                    anchors.margins: 10
                    anchors.topMargin: 39

                    GridLayout {
                        anchors.fill: parent
                        rows: 2
                        columns: 1

                        RowLayout {
                            id: rowMessages
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 300
                            Layout.column: 0
                            Layout.row: 0

                            ScrollView
                            {
                                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                clip: true
                                leftPadding: 10
                                topPadding: 5
                                rightPadding: 10
                                bottomPadding: 5
                                visible: tabId === currentTab


                                ListView {
                                    id: listView
                                    model: attributes
                                    delegate: Rectangle {
                                        anchors.left: listView.contentItem.left
                                        anchors.right: listView.contentItem.right
                                        height: text.contentHeight
                                        color: 'transparent'

                                        TextEdit {
                                            id: text
                                            text: message
                                            readOnly: true
                                            selectByMouse: true
                                            selectionColor: '#0164AD'
                                            selectedTextColor: '#ffffff'
                                            width: parent.width
                                            wrapMode: Text.Wrap
                                            font.family: robotoMono.name
                                            renderType: Text.NativeRendering
                                            font.pixelSize: 13
                                            color: msgColor || '#ffffff'
                                        }
                                    }
                                    onCountChanged: {
                                        var newIndex = count - 1
                                        positionViewAtEnd()
                                        currentIndex = newIndex
                                    }
                                }
                            }
                        }

                        RowLayout {
                            id: commandLine
                            Layout.fillWidth: true
                            clip: true
                            Layout.column: 0
                            Layout.row: 1
                            Layout.maximumHeight: 30
                            Layout.minimumHeight: 30

                            TextField {
                                id: cliTextField
                                font.pixelSize: 13
                                font.family: robotoMono.name
                                renderType: Text.NativeRendering
                                color: '#ffffff'
                                selectionColor: "#0164AD"
                                selectedTextColor: "#ffffff"
                                topPadding: 0
                                padding: 6
                                Layout.bottomMargin: -5
                                Layout.rightMargin: -5
                                Layout.leftMargin: -5
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                selectByMouse: true
                                background: Rectangle {
                                    color: 'transparent'
                                    border.color: '#5C5C5C'
                                }
                                Keys.onPressed: {
                                    if(event.key === Qt.Key_Escape) {
                                        cliTextField.clear()
                                    }
                                    if(event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                                        var cmd = cliTextField.text.split(" ");

                                        try {

                                            if(!simConnected) {
                                                throw "X-Plane connection not established."
                                            }

                                            switch(currentTab) {
                                            case 0:
                                                // radio messages
                                                switch(cmd[0].toLowerCase())
                                                {
                                                case ".clear":
                                                    clearMessages()
                                                    cliTextField.clear()
                                                    break;
                                                case ".msg":
                                                case ".chat":
                                                    break;
                                                case ".wallop":
                                                    break;
                                                case ".wx":
                                                case ".metar":
                                                    break;
                                                case ".atis":
                                                    break;
                                                case ".com1":
                                                case ".com2":
                                                    if (!/^1\d\d[\.\,]\d{1,3}$/.test(cmd[1])) {
                                                        throw "Invalid frequency format.";
                                                    }
                                                    var freq = FrequencyUtils.frequencyToInt(cmd[1])
                                                    var radio = cmd[0].toLowerCase() === ".com1" ? 1 : 2
                                                    setRadioStack(radio, freq);
                                                    cliTextField.clear()
                                                    break;
                                                case ".tx":
                                                    break;
                                                case ".rx":
                                                    break;
                                                case ".x":
                                                case ".xpndr":
                                                case ".xpdr":
                                                case ".squawk":
                                                    if (!/^[0-7]{4}$/.test(cmd[1])) {
                                                        throw "Invalid transponder code format.";
                                                    }
                                                    var code = parseInt(cmd[1])
                                                    setTransponderCode(code)
                                                    cliTextField.clear()
                                                    break;
                                                case ".towerview":
                                                    break;
                                                default:
                                                    appendMessage(cliTextField.text)
                                                    cliTextField.clear()
                                                    break;
                                                }
                                                break;
                                            case 1:
                                                // notes
                                                switch(cmd[0].toLowerCase())
                                                {
                                                case ".clear":
                                                    clearNotes()
                                                    cliTextField.clear()
                                                    break;
                                                default:
                                                    appendNote(cliTextField.text)
                                                    cliTextField.clear()
                                                    break;
                                                }
                                                break;
                                            default:
                                                // private message
                                                break;
                                            }
                                        }
                                        catch(err) {
                                            appendErrorMessage(err)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Repeater {
                model: cliModel
                delegate: cliDelegate
                anchors.fill: parent
                anchors.margins: 10
            }
        }
    }

    MouseArea {
        id: mouseArea
        property real lastMouseX: 0
        property real lastMouseY: 0
        property int activeEdges: 0
        property int gutterSize: 8
        property bool moveable: false
        hoverEnabled: true
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton

        cursorShape: {
            const p = Qt.point(mouseX, mouseY)
            if (p.x < gutterSize && p.y < gutterSize)
                return Qt.SizeFDiagCursor
            if (p.x >= width - gutterSize && p.y >= height - gutterSize)
                return Qt.SizeFDiagCursor
            if (p.x >= width - gutterSize && p.y < gutterSize)
                return Qt.SizeBDiagCursor
            if (p.x < gutterSize && p.y >= height - gutterSize)
                return Qt.SizeBDiagCursor
            if (p.x < gutterSize || p.x >= width - gutterSize)
                return Qt.SizeHorCursor
            if (p.y < gutterSize || p.y >= height - gutterSize)
                return Qt.SizeVerCursor
        }

        onDoubleClicked: {
            if (mainWindow.visibility == Window.Maximized) {
                mainWindow.showNormal()
            } else {
                mainWindow.showMaximized()
            }
            moveable = false
        }

        onPressed: {
            if (mouse.button !== Qt.LeftButton
                    || mainWindow.visibility === Window.Maximized
                    || mainWindow.visibility === Window.FullScreen) {
                return
            }
            activeEdges = 0
            if (mouseX < gutterSize)
                activeEdges |= Qt.LeftEdge
            if (mouseY < gutterSize)
                activeEdges |= Qt.TopEdge
            if (mouseX > mainWindow.width - gutterSize)
                activeEdges |= Qt.RightEdge
            if (mouseY > mainWindow.height - gutterSize)
                activeEdges |= Qt.BottomEdge

            if (mainWindow.startSystemMove != undefined
                    && Qt.platform.os !== "osx") {
                if (activeEdges == 0) {
                    mainWindow.startSystemMove()
                } else {
                    mainWindow.startSystemResize(activeEdges)
                }
            } else {
                lastMouseX = mouseX
                lastMouseY = mouseY
                moveable = (activeEdges === 0)
            }
        }

        onReleased: {
            activeEdges = 0
            moveable = false
        }

        onMouseXChanged: {
            // Use system native move & resize on Qt >= 5.15
            if (mainWindow.startSystemMove != undefined
                    && Qt.platform.os !== "osx") {
                return
            }

            if (mainWindow.visibility == Window.Maximized
                    || mainWindow.visibility == Window.FullScreen || !pressed) {
                return
            }

            if (activeEdges & Qt.LeftEdge) {
                mainWindow.width -= (mouseX - lastMouseX)
                if (mainWindow.width < window.minimumWidth) {
                    mainWindow.width = window.minimumWidth
                } else {
                    mainWindow.x += (mouseX - lastMouseX)
                }
            } else if (activeEdges & Qt.RightEdge) {
                mainWindow.width += (mouseX - lastMouseX)
                if (mainWindow.width < window.minimumWidth) {
                    mainWindow.width = window.minimumWidth
                }
                lastMouseX = mouseX
            } else if (moveable) {
                mainWindow.x += (mouseX - lastMouseX)
            }
        }

        onMouseYChanged: {
            // Use system native move & resize on Qt >= 5.15
            if (mainWindow.startSystemMove != undefined
                    && Qt.platform.os !== "osx") {
                return
            }

            if (mainWindow.visibility == Window.Maximized
                    || mainWindow.visibility == Window.FullScreen || !pressed) {
                return
            }

            if (activeEdges & Qt.TopEdge) {
                mainWindow.height -= (mouseY - lastMouseY)
                if (mainWindow.height < window.minimumHeight) {
                    mainWindow.height = window.minimumHeight
                } else {
                    mainWindow.y += (mouseY - lastMouseY)
                }
            } else if (activeEdges & Qt.BottomEdge) {
                mainWindow.height += (mouseY - lastMouseY)
                if (mainWindow.height < window.minimumHeight) {
                    mainWindow.height = window.minimumHeight
                }
                lastMouseY = mouseY
            } else if (moveable) {
                mainWindow.y += (mouseY - lastMouseY)
            }
        }
    }
}
