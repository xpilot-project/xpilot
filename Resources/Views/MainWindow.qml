import QtQuick 2.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import QtWebSockets 1.2
import QtMultimedia 5.12
import QtQuick.Dialogs 1.2

import AppConfig 1.0
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
    property int currentTab
    property bool closing: false
    property bool networkConnected: false
    property string ourCallsign: ""
    property bool initialized: false
    property bool simConnected: false

    property string colorGreen: "#85a664"
    property string colorOrange: "#ffa500"
    property string colorWhite: "#ffffff"
    property string colorGray: "#c0c0c0"
    property string colorYellow: "#ffff00"
    property string colorRed: "#eb2f06"
    property string colorCyan: "#00ffff"
    property string colorBrightGreen: "#00c000"

    FontLoader {
        id: ubuntuRegular
        source: "../Fonts/Ubuntu-Regular.ttf"
    }

    FontLoader {
        id: robotoMono
        source: "../Fonts/Roboto-Mono.ttf"
    }

    SoundEffect {
        id: alertSound
        source: "../Sounds/Alert.wav"
    }

    SoundEffect {
        id: broadcastSound
        source: "../Sounds/Broadcast.wav"
    }

    SoundEffect {
        id: directRadioMessageSound
        source: "../Sounds/DirectRadioMessage.wav"
    }

    SoundEffect {
        id: errorSound
        source: "../Sounds/Error.wav"
    }


    SoundEffect {
        id: newMessageSound
        source: "../Sounds/NewMessage.wav"
    }


    SoundEffect {
        id: privateMessageSound
        source: "../Sounds/PrivateMessage.wav"
    }


    SoundEffect {
        id: radioMessageSound
        source: "../Sounds/RadioMessage.wav"
    }


    SoundEffect {
        id: selcalSound
        source: "../Sounds/SelCal.wav"
    }

    MessageDialog {
        id: confirmClose
        title: "Confirm Close"
        text: "You are still connected to the network. Are you sure you want to close xPilot?"
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: {
            closing = true
            mainWindow.close()
        }
    }

    Component.onCompleted: {
        appendMessage("Waiting for X-Plane connection... Please make sure X-Plane is running and a flight is loaded.", colorYellow);
        width = AppConfig.WindowConfig.Width;
        height = AppConfig.WindowConfig.Height;
        x = AppConfig.WindowConfig.X;
        y = AppConfig.WindowConfig.Y;
        initialized = true;
    }

    // @disable-check M16
    onClosing: {
        close.accepted = !networkConnected || closing
        onTriggered: if(!closing && networkConnected) confirmClose.open()
    }

    onXChanged: {
        if(initialized) {
            AppConfig.WindowConfig.X = x;
        }
    }

    onYChanged: {
        if(initialized) {
            AppConfig.WindowConfig.Y = y;
        }
    }

    onHeightChanged: {
        if(initialized) {
            AppConfig.WindowConfig.Height = height;
        }
    }

    onWidthChanged: {
        if(initialized) {
            AppConfig.WindowConfig.Width = width;
        }
    }

    Connections {
        target: connectWindow
        function onCloseWindow() {
            connectWindow.destroy()
        }
    }

    Connections {
        target: settingsWindow
        function onCloseWindow() {
            settingsWindow.destroy()
        }
    }

    Connections {
        target: appCore

        function onServerListDownloaded(count) {
            appendMessage(`Server list download succeeded. ${count} servers found.`, colorYellow)
        }

        function onServerListDownloadError(count) {
            appendMessage("Server list download failed. Using previously-cached server list.", colorRed)
        }
    }

    Connections {
        target: xplaneAdapter

        function onSimConnectionStateChanged(state) {
            if(!simConnected && state) {
                appendMessage("X-Plane connection established.", colorYellow)
            }
            else if(simConnected && !state) {
                networkManager.disconnectFromNetwork()
                appendMessage("X-Plane connection lost. Please make sure X-Plane is running and a flight is loaded.", colorRed)
                mainWindow.alert(0);
                errorSound.play()
            }
            simConnected = state;
        }

        function onReplayModeDetected() {
            if(networkConnected) {
                networkManager.disconnectFromNetwork()
                appendMessage("You have been disconnected from the network becasue Replay Mode is enabled.", colorRed)
                mainWindow.alert(0);
                errorSound.play()
            }
        }
    }

    Connections {
        target: audio

        function onNotificationPosted(type, message) {
            switch(type) {
            case 0: // info
                appendMessage(message, colorYellow)
                break;
            case 1: // warning
                appendMessage(message, colorOrange)
                break;
            case 2: // error
                appendMessage(message, colorRed)
                break;
            case 3: // text message
                appendMessage(message, colorGray);
                break;
            default:
                appendMessage(message, colorYellow);
                break;
            }
        }
    }

    Connections {
        target: networkManager

        function onNetworkConnected(callsign) {
            ourCallsign = callsign
            networkConnected = true;
        }

        function onNetworkDisconnected() {
            networkConnected = false;
            nearbyEnroute.clear()
            nearbyApproach.clear()
            nearbyTower.clear()
            nearbyGround.clear()
            nearbyDelivery.clear()
            nearbyAtis.clear()
            nearbyObservers.clear()
        }

        function onNotificationPosted(type, message) {
            switch(type) {
            case 0: // info
                appendMessage(message, colorYellow)
                break;
            case 1: // warning
                appendMessage(message, colorOrange)
                break;
            case 2: // error
                appendMessage(message, colorRed)
                break;
            case 3: // text message
                appendMessage(message, colorGray);
                break;
            default:
                appendMessage(message, colorYellow);
                break;
            }
        }

        function onServerMessageReceived(message) {
            appendMessage(`SERVER: ${message}`, colorGreen)
        }

        function onBroadcastMessageReceived(from, message) {
            appendMessage(`[BROADCAST] ${from}: ${message}`, colorOrange)
            broadcastSound.play()
            mainWindow.alert(0)
        }

        function onSelcalAlertReceived(from, frequencies) {
            appendMessage(`SELCAL alert received on ${FrequencyUtils.formatFromFsd(frequencies[0])}`, colorYellow)
            selcalSound.play()
            mainWindow.alert(0)
        }

        function onRealNameReceived(callsign, name) {
            var idx = getChatTabIndex(callsign)
            if(idx > 0) {
                var model = cliModel.get(idx)
                if(!model.realName) {
                    model.attributes.insert(0, {message:`Name: ${name}`, msgColor: colorYellow})
                    model.realName = true
                }
            }
        }

        function onPrivateMessageReceived(from, message) {
            var tab = getChatTabIndex(from)
            if(tab === null) {
                createChatTab(from)
                tab = getChatTabIndex(from)
                appendPrivateMessage(tab, message, from, colorWhite)
                mainWindow.alert(0)
                newMessageSound.play()
            }
            else {
                if(currentTab !== tab) {
                    markTabUnread(from)
                }
                appendPrivateMessage(tab, message, from, colorWhite)
                mainWindow.alert(0)
                privateMessageSound.play()
            }
        }

        function onPrivateMessageSent(to, message) {
            var tab = getChatTabIndex(to)
            if(tab !== null) {
                appendPrivateMessage(tab, message, ourCallsign, colorCyan)
            }
        }

        function onRadioMessageReceived(args) {
            var message = "";
            if(args.DualReceiver) {
                var freqString = "";
                if(args.Frequencies.length > 1) {
                    freqString = `${FrequencyUtils.formatFromFsd(args.Frequencies[0])} & ${FrequencyUtils.formatFromFsd(args.Frequencies[1])}`;
                }
                else {
                    freqString = FrequencyUtils.formatFromFsd(args.Frequencies[0]);
                }
                message = `${args.From} on ${freqString}: ${args.Message}`;
            }
            else {
                message = `${args.From}: ${args.Message}`;
            }

            appendMessage(message, args.IsDirect ? colorWhite : colorGray)

            if(args.IsDirect) {
                directRadioMessageSound.play();
                mainWindow.alert(0);
            }
            else {
                radioMessageSound.play();
            }
        }

        function onControllerAtisReceived(callsign, atis) {
            appendMessage(`${callsign} ATIS:`, colorBrightGreen)
            atis.forEach(function(line) {
                appendMessage(line, colorBrightGreen)
            })
        }

        function onMetarReceived(station, metar) {
            appendMessage(`METAR: ${metar}`, colorBrightGreen)
        }
    }

    Connections {
        target: controllerManager

        function findController(myModel, callsign) {
            for(var i = 0; i < myModel.count; i++) {
                var element = myModel.get(i);
                if(callsign === element.Callsign) {
                    return i;
                }
            }
            return -1;
        }

        function onControllerAdded(controller) {
            var idx;
            if(controller.Callsign.endsWith("_CTR") || controller.Callsign.endsWith("_FSS")) {
                idx = findController(nearbyEnroute, controller.Callsign)
                if(idx < 0) {
                    nearbyEnroute.append(controller)
                }
            }
            else if(controller.Callsign.endsWith("_APP") || controller.Callsign.endsWith("_DEP")) {
                idx = findController(nearbyApproach, controller.Callsign)
                if(idx < 0) {
                    nearbyApproach.append(controller)
                }
            }
            else if(controller.Callsign.endsWith("_TWR")) {
                idx = findController(nearbyTower, controller.Callsign)
                if(idx < 0) {
                    nearbyTower.append(controller)
                }
            }
            else if(controller.Callsign.endsWith("_GND")) {
                idx = findController(nearbyGround, controller.Callsign)
                if(idx < 0) {
                    nearbyGround.append(controller)
                }
            }
            else if(controller.Callsign.endsWith("_DEL")) {
                idx = findController(nearbyDelivery, controller.Callsign)
                if(idx < 0) {
                    nearbyDelivery.append(controller)
                }
            }
            else if(controller.Callsign.endsWith("_ATIS")) {
                idx = findController(nearbyAtis, controller.Callsign)
                if(idx < 0) {
                    nearbyAtis.append(controller)
                }
            }
            else {
                idx = findController(nearbyObservers, controller.Callsign)
                if(idx < 0) {
                    nearbyObservers.append(controller)
                }
            }
        }

        function onControllerDeleted(controller) {
            var idx;
            if(controller.Callsign.endsWith("_CTR") || controller.Callsign.endsWith("_FSS")) {
                idx = findController(nearbyEnroute, controller.Callsign)
                if(idx >= 0) {
                    nearbyEnroute.remove(idx)
                }
            }
            else if(controller.Callsign.endsWith("_APP") || controller.Callsign.endsWith("_DEP")) {
                idx = findController(nearbyApproach, controller.Callsign)
                if(idx >= 0) {
                    nearbyApproach.remove(idx)
                }
            }
            else if(controller.Callsign.endsWith("_TWR")) {
                idx = findController(nearbyTower, controller.Callsign)
                if(idx >= 0) {
                    nearbyTower.remove(idx)
                }
            }
            else if(controller.Callsign.endsWith("_GND")) {
                idx = findController(nearbyGround, controller.Callsign)
                if(idx >= 0) {
                    nearbyGround.remove(idx)
                }
            }
            else if(controller.Callsign.endsWith("_DEL")) {
                idx = findController(nearbyDelivery, controller.Callsign)
                if(idx >= 0) {
                    nearbyDelivery.remove(idx)
                }
            }
            else if(controller.Callsign.endsWith("_ATIS")) {
                idx = findController(nearbyAtis, controller.Callsign)
                if(idx >= 0) {
                    nearbyAtis.remove(idx)
                }
            }
            else {
                idx = findController(nearbyObservers, controller.Callsign)
                if(idx >= 0) {
                    nearbyObservers.remove(idx)
                }
            }
        }
    }

    function appendMessage(message, color = colorGray) {
        var model = cliModel.get(0)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`, msgColor: color})
    }

    function appendPrivateMessage(tab, message, from, color = colorCyan) {
        var model = cliModel.get(tab)
        if(from) {
            model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${from}: ${message}`, msgColor: color})
        }
        else {
            model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`, msgColor: color})
        }
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

    function getChatTabIndex(callsign) {
        for(var i = 0; i < tabModel.count; i++) {
            if(tabModel.get(i).title.toUpperCase() === callsign.toUpperCase()) {
                return i
            }
        }
        return null
    }

    function createChatTab(callsign) {
        tabModel.append({title: callsign.toUpperCase(), disposable: true, hasUnread: false})
        var idx = getChatTabIndex(callsign)
        cliModel.append({tabId: idx, attributes: [], realName: false})
        tabListView.currentIndex = idx
        currentTab = idx
        networkManager.requestRealName(callsign)
    }

    function focusOrCreateTab(callsign) {
        var idx = getChatTabIndex(callsign)
        if(idx === null) {
            createChatTab(callsign)
        }
        else {
            tabListView.currentIndex = idx
            currentTab = idx
        }
    }

    function markTabUnread(callsign) {
        for(var i = 0; i < tabModel.count; i++) {
            var tab = tabModel.get(i)
            if(tab.title.toUpperCase() === callsign.toUpperCase()) {
                tab.hasUnread = true
            }
        }
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
                onStartChatSession: {
                    focusOrCreateTab(callsign)
                }
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
                    hasUnread: false
                }
                ListElement {
                    title: "Notes"
                    disposable: false
                    hasUnread: false
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
                        color: itemIndex === view.currentIndex ? "white" : (hasUnread ? "yellow" : frameColor)
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
                            onClicked: {
                                view.currentIndex = 0
                                currentTab = 0
                                cliModel.remove(itemIndex)
                                tabModel.remove(itemIndex)
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onClicked: {
                            hasUnread = false
                            currentTab = itemIndex
                            view.currentIndex = itemIndex
                        }
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }

            ListView {
                id: tabListView
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

                                onFocusChanged: {
                                    cliTextField.forceActiveFocus()
                                }

                                ListView {
                                    id: listView
                                    model: attributes
                                    delegate: Rectangle {
                                        anchors.left: listView.contentItem.left
                                        anchors.right: listView.contentItem.right
                                        height: textHistory.contentHeight
                                        color: 'transparent'

                                        Text {
                                            id: textHistory
                                            text: message
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
                                        var cmd = cliTextField.text.split(" ").filter(function(i) { return i })

                                        try {

                                            if(!simConnected) {
                                                throw "X-Plane connection not found. Please make sure X-Plane is running and a flight is loaded."
                                            }

                                            if(currentTab == 0) {
                                                if(cliTextField.text.startsWith(".")) {
                                                    // radio messages
                                                    switch(cmd[0].toLowerCase())
                                                    {
                                                    case ".clear":
                                                        clearMessages()
                                                        cliTextField.clear()
                                                        break;
                                                    case ".chat":
                                                        if(cmd.length < 2) {
                                                            throw "Not enough parameters. Expected .chat CALLSIGN"
                                                        }
                                                        if(cmd[1].length > 10) {
                                                            throw "Callsign too long."
                                                        }
                                                        focusOrCreateTab(cmd[1])
                                                        cliTextField.clear()
                                                        break;
                                                    case ".msg":
                                                        if(cmd.length < 3) {
                                                            throw "Not enough parameters. Expected .msg CALLSIGN MESSAGE"
                                                        }
                                                        if(!networkConnected) {
                                                            throw "Not connected to network."
                                                        }
                                                        if(cmd[1].length > 10) {
                                                            throw "Callsign too long."
                                                        }
                                                        focusOrCreateTab(cmd[1])
                                                        networkManager.sendPrivateMessage(cmd[1], cmd.slice(2).join(" "))
                                                        cliTextField.clear()
                                                        break;
                                                    case ".wallop":
                                                        if(cmd.length < 2) {
                                                            throw "Not enough parameters. Expected .wallop MESSAGE"
                                                        }
                                                        break;
                                                    case ".wx":
                                                    case ".metar":
                                                        if(cmd.length < 2) {
                                                            throw `Not enough parameters. Expected ${cmd[0]} STATION-ID`
                                                        }
                                                        networkManager.requestMetar(cmd[1])
                                                        cliTextField.clear();
                                                        break;
                                                    case ".atis":
                                                        if(cmd.length < 2) {
                                                            throw `Not enough parameters. Expected .atis CALLSIGN`
                                                        }
                                                        break;
                                                    case ".com1":
                                                    case ".com2":
                                                        if (!/^1\d\d[\.\,]\d{1,3}$/.test(cmd[1])) {
                                                            throw "Invalid frequency format.";
                                                        }
                                                        var freq = FrequencyUtils.frequencyToInt(cmd[1])
                                                        var radio = cmd[0].toLowerCase() === ".com1" ? 1 : 2
                                                        if(radio === 1) {
                                                            xplaneAdapter.setCom1Frequency(freq);
                                                        }
                                                        else {
                                                            xplaneAdapter.setCom2Frequency(freq);
                                                        }
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
                                                    case ".sq":
                                                        if(cmd.length < 2) {
                                                            throw `Not enough parameters. Expected ${cmd[0]} SQUAWK-CODE`
                                                        }
                                                        if (!/^[0-7]{4}$/.test(cmd[1])) {
                                                            throw "Invalid transponder code format.";
                                                        }
                                                        var code = parseInt(cmd[1])
                                                        xplaneAdapter.setTransponderCode(code)
                                                        cliTextField.clear()
                                                        break;
                                                    case ".towerview":
                                                        break;
                                                    default:
                                                        throw `Unknown command: ${cmd[0].toLowerCase()}`
                                                    }
                                                }
                                                else {
                                                    if(!networkConnected) {
                                                        throw "Not connected to network."
                                                    }
                                                    networkManager.sendRadioMessage(cliTextField.text)
                                                    appendMessage(cliTextField.text, colorCyan)
                                                    cliTextField.clear()
                                                }
                                            }
                                            else if(currentTab == 1) {
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
                                            }
                                            else {
                                                if(!networkConnected) {
                                                    appendPrivateMessage(currentTab, "Not connected to network.", "", colorRed)
                                                    errorSound.play()
                                                }
                                                else {
                                                    var callsign = tabModel.get(currentTab).title;
                                                    networkManager.sendPrivateMessage(callsign, cliTextField.text)
                                                    cliTextField.clear()
                                                }
                                            }
                                        }
                                        catch(err) {
                                            appendMessage(err, colorRed)
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
