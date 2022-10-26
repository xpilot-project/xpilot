import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import QtQuick.Layouts
import QtMultimedia

import AppConfig 1.0
import "../Scripts/FrequencyUtils.js" as FrequencyUtils
import "../Scripts/TimestampUtils.js" as TimestampUtils
import "../Scripts/StringUtils.js" as StringUtils
import "../Components"
import "../Components/DownloadCSLModels"
import "../Components/VersionCheck"
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

    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2

    property QtObject connectWindow
    property QtObject settingsWindow
    property QtObject downloadCslWindow // csl install
    property QtObject setXplanePathWindow // csl install
    property QtObject extractCslModelsWindow // csl install
    property int currentTab
    property bool networkConnected: false
    property string ourCallsign: ""
    property bool initialized: false
    property bool simConnected: false
    property var radioStackState

    property string colorGreen: "#85a664"
    property string colorOrange: "#ffa500"
    property string colorWhite: "#ffffff"
    property string colorGray: "#c0c0c0"
    property string colorYellow: "#ffff00"
    property string colorRed: "#eb2f06"
    property string colorCyan: "#00ffff"
    property string colorBrightGreen: "#00c000"

    FontLoader {
        id: robotoMono
        source: "../Fonts/Roboto-Mono.ttf"
    }

    SoundEffect {
        id: alertSound
        source: "file:" + appDataPath + "Sounds/Alert.wav"
    }

    SoundEffect {
        id: broadcastSound
        source: "file:" + appDataPath + "Sounds/Broadcast.wav"
    }

    SoundEffect {
        id: directRadioMessageSound
        source: "file:" + appDataPath + "Sounds/DirectRadioMessage.wav"
    }

    SoundEffect {
        id: errorSound
        source: "file:" + appDataPath + "Sounds/Error.wav"
    }

    SoundEffect {
        id: newMessageSound
        source: "file:" + appDataPath + "Sounds/NewMessage.wav"
    }

    SoundEffect {
        id: privateMessageSound
        source: "file:" + appDataPath + "Sounds/PrivateMessage.wav"
    }

    SoundEffect {
        id: radioMessageSound
        source: "file:" + appDataPath + "Sounds/RadioMessage.wav"
    }

    SoundEffect {
        id: selcalSound
        source: "file:" + appDataPath + "Sounds/SELCAL.wav"
    }

    NewVersionAvailable {
        id: modal_newVersionAvailable
    }

    DownloadingUpdate {
        id: modal_downlodingUpdate;
    }

    DisconnectDialog {
        id: confirmClose
        onExitApplication: Qt.exit(0)
    }

    MicrophoneCalibrationRequired {
        id: microphoneCalibrationRequired
    }

    Connections {
        target: AppConfig

        function onPermissionError(error) {
            appendMessage("Configuration Error: " + error, colorRed)
            errorSound.play()
        }

        function onSettingsChanged() {
            if(AppConfig.KeepWindowVisible) {
                mainWindow.flags |= Qt.WindowStaysOnTopHint
            } else {
                mainWindow.flags &= ~Qt.WindowStaysOnTopHint
            }
        }

        function onInputDeviceChanged() {
            AppConfig.MicrophoneCalibrated = false
            AppConfig.saveConfig()
        }
    }

    Component.onCompleted: {
        // we call this again so we can present the permission error if applicable
        AppConfig.loadConfig()

        appendMessage(`Welcome to xPilot v${appVersion}`, colorYellow)
        if(AppConfig.XplaneNetworkAddress !== "127.0.0.1" && AppConfig.XplaneNetworkAddress !== "localhost") {
            appendMessage(`Waiting for X-Plane connection (${AppConfig.XplaneNetworkAddress})... Please make sure X-Plane is running and a flight is loaded.`, colorYellow)
        } else {
            appendMessage("Waiting for X-Plane connection... Please make sure X-Plane is running and a flight is loaded.", colorYellow)
        }

        if(!AppConfig.SilenceModelInstall) {
            createCslDownloadWindow()
        }

        if(AppConfig.KeepWindowVisible) {
            mainWindow.flags |= Qt.WindowStaysOnTopHint
        }

        if(AppConfig.WindowConfig.Maximized) {
            mainWindow.showFullScreen()
        }
        else {
            width = Math.max(minimumWidth, AppConfig.WindowConfig.Width)
            height = Math.max(minimumHeight, AppConfig.WindowConfig.Height)
            x = AppConfig.WindowConfig.X;
            y = AppConfig.WindowConfig.Y;
        }
        initialized = true;
    }

    onClosing: function(close) {
        close.accepted = !networkConnected
        onTriggered: if(networkConnected) confirmClose.open()
    }

    onXChanged: {
        if(initialized) {
            if(visibility === Window.Maximized || visibility === Window.FullScreen) {
                return;
            }
            AppConfig.WindowConfig.X = x;
            AppConfig.saveConfig()
        }
    }

    onYChanged: {
        if(initialized) {
            if(visibility === Window.Maximized || visibility === Window.FullScreen) {
                return;
            }
            AppConfig.WindowConfig.Y = y;
            AppConfig.saveConfig()
        }
    }

    onVisibilityChanged: function(visibility) {
        if(initialized && visibility !== Window.Hidden) {
            var isMaximized = (visibility === Window.Maximized || visibility === Window.FullScreen)
            if(AppConfig.WindowConfig.Maximized && !isMaximized) {
                width = minimumWidth
                height = minimumHeight
                var center = Qt.point(Screen.width / 2 - width / 2, Screen.height / 2 - height / 2)
                x = center.x
                y = center.y
                AppConfig.WindowConfig.X = center.x
                AppConfig.WindowConfig.Y = center.y
                AppConfig.WindowConfig.Width = width
                AppConfig.WindowConfig.Height = height
            }
            AppConfig.WindowConfig.Maximized = isMaximized
            AppConfig.saveConfig()
        }
    }

    onHeightChanged: {
        if(visibility === Window.Maximized || visibility === Window.FullScreen) {
            return;
        }

        if(initialized) {
            AppConfig.WindowConfig.Height = height;
            AppConfig.saveConfig()
        }
    }

    onWidthChanged: {
        if(visibility === Window.Maximized || visibility === Window.FullScreen) {
            return;
        }

        if(initialized) {
            AppConfig.WindowConfig.Width = width;
            AppConfig.saveConfig()
        }
    }

    Connections {
        target: versionCheck

        function onNewVersionAvailable() {
            modal_newVersionAvailable.open()
        }

        function onDownloadStarted() {
            modal_downlodingUpdate.open()
        }

        function onNoUpdatesAvailable() {
            appendMessage("Version check complete. You are running the latest version of xPilot.", colorYellow)
        }

        function onErrorEncountered(error) {
            appendMessage(error, colorRed)
            errorSound.play()
        }
    }

    Connections {
        target: typeCodeDatabase

        function onTypeCodeDownloadError(error) {
            appendMessage(error, colorRed)
            errorSound.play()
        }
    }

    Connections {
        target: installModels

        function onSetXplanePath() {
            downloadCslWindow.destroy()
            createSetXplanePathWindow()
        }

        function onValidXplanePath() {
            setXplanePathWindow.destroy()
            createExtractCslModelsWindow()
        }

        function onUnzipFinished() {
            extractCslModelsWindow.destroy()
            appendMessage("CSL aircraft model package successfully installed! Please restart xPilot and X-Plane before connecting to the network.", colorYellow);
            AppConfig.SilenceModelInstall = true
        }

        function onDownloadProgressChanged(val) {
            downloadCslWindow.pctProgress = val
        }

        function onUnzipProgressChanged(val) {
            extractCslModelsWindow.pctProgress = val
        }

        function onErrorEncountered(error) {
            downloadCslWindow.destroy()
            setXplanePathWindow.destroy()
            extractCslModelsWindow.destroy()
            appendMessage(error, colorRed);
            errorSound.play()
        }
    }

    Connections {
        target: downloadCslWindow
        function onCloseWindow() {
            downloadCslWindow.destroy()
        }
    }

    Connections {
        target: setXplanePathWindow
        function onCloseWindow() {
            setXplanePathWindow.destroy()
        }
    }

    Connections {
        target: extractCslModelsWindow
        function onCloseWindow() {
            extractCslModelsWindow.destroy()
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
        target: serverListManager

        function onServerListDownloaded(count) {
            appendMessage(`Server list download succeeded. ${count} servers found.`, colorYellow)
        }

        function onServerListDownloadError(count) {
            appendMessage("Server list download failed. Using previously-cached server list.", colorRed)
        }
    }

    Connections {
        target: xplaneAdapter

        function onRadioStackStateChanged(stack) {
            if(radioStackState !== stack) {
                radioStackState = stack;
            }
        }

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
                appendMessage("You have been disconnected from the network because Replay Mode is enabled.", colorRed)
                if(AppConfig.AlertDisconnect) {
                    mainWindow.alert(0)
                    errorSound.play()
                }
            }
        }

        function onInvalidPluginVersion() {
            appendMessage("Unsupported xPilot plugin version detected. Please close X-Plane and reinstall the latest version of xPilot.", colorRed)
            errorSound.play()
            mainWindow.alert(0)
        }

        function onInvalidCslConfiguration() {
            appendMessage("No valid CSL paths are configured or enabled, or you have no CSL models installed. Please verify the CSL configuration in X-Plane (Plugins > xPilot > Settings). If you need assistance configuring your CSL paths, see the \"CSL Configuration\" section in the xPilot Documentation (http://beta.xpilot-project.org). Restart X-Plane and xPilot after you have properly configured your CSL models.", colorRed)
            errorSound.play()
            mainWindow.alert(0)
        }

        function onRadioMessageSent(message) {
            appendMessage(message, colorCyan)
        }

        function onAircraftIgnored(callsign) {
            appendMessage(`${callsign} has been added to the ignore list.`, colorYellow)
        }

        function onAircraftAlreadyIgnored(callsign) {
            appendMessage(`${callsign} is already in the ignore list.`, colorRed)
            errorSound.play()
        }

        function onAircraftUnignored(callsign) {
            appendMessage(`${callsign} has been removed from the ignore list.`, colorYellow)
        }

        function onAircraftNotIgnored(callsign) {
            appendMessage(`${callsign} was not found in the ignore list.`, colorRed)
            errorSound.play()
        }

        function onIgnoreList(list) {
            if(list.length > 0) {
                appendMessage(`The following aircraft are currently ignored: ${list.join(", ")}`, colorYellow)
            }
            else {
                appendMessage("The ignore list is currently empty.", colorYellow)
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

        function onMicrophoneCalibrationRequired() {
            microphoneCalibrationRequired.open()
        }

        function onNetworkConnected(callsign) {
            ourCallsign = callsign
            networkConnected = true;
        }

        function onNetworkDisconnected(forced) {
            networkConnected = false;
            nearbyEnroute.clear()
            nearbyApproach.clear()
            nearbyTower.clear()
            nearbyGround.clear()
            nearbyDelivery.clear()
            nearbyAtis.clear()

            if(forced && AppConfig.AlertDisconnect) {
                errorSound.play()
                mainWindow.alert(0)
            }
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
            if(AppConfig.AlertNetworkBroadcast) {
                broadcastSound.play()
                mainWindow.alert(0)
            }
        }

        function onWallopSent(message) {
            appendMessage(`[WALLOP] ${message}`, colorRed)
        }

        function onSelcalAlertReceived(from, frequencies) {
            appendMessage(`SELCAL alert received on ${FrequencyUtils.fromNetworkFormat(frequencies[0])}`, colorYellow)
            if(AppConfig.AlertSelcal) {
                mainWindow.alert(0)
                if(!radioStackState.SelcalMuteOverride) {
                    selcalSound.play()
                }
            }
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
                if(AppConfig.AlertPrivateMessage) {
                    mainWindow.alert(0)
                    newMessageSound.play()
                }
            }
            else {
                if(currentTab !== tab) {
                    markTabUnread(from)
                }
                appendPrivateMessage(tab, message, from, colorWhite)
                if(AppConfig.AlertPrivateMessage) {
                    mainWindow.alert(0)
                    privateMessageSound.play()
                }
            }
        }

        function onPrivateMessageSent(to, message) {
            var tab = getChatTabIndex(to)
            if(tab !== null) {
                appendPrivateMessage(tab, message, ourCallsign, colorCyan)
            }
            else {
                createChatTab(to)
                tab = getChatTabIndex(to)
                appendPrivateMessage(tab, message, ourCallsign, colorCyan)
            }
        }

        function onRadioMessageReceived(args) {
            var message = "";
            if(args.DualReceiver) {
                var freqString = "";
                if(args.Frequencies.length > 1) {
                    freqString = `${FrequencyUtils.fromNetworkFormat(args.Frequencies[0])} & ${FrequencyUtils.fromNetworkFormat(args.Frequencies[1])}`;
                }
                else {
                    freqString = FrequencyUtils.fromNetworkFormat(args.Frequencies[0]);
                }
                message = `${args.From} on ${freqString}: ${args.Message}`;
            }
            else {
                message = `${args.From}: ${args.Message}`;
            }

            appendMessage(message, args.IsDirect ? colorWhite : colorGray)

            if(args.IsDirect) {
                if(AppConfig.AlertDirectRadioMessage) {
                    mainWindow.alert(0);
                    directRadioMessageSound.play();
                }
            }
            else {
                if(AppConfig.AlertRadioMessage) {
                    radioMessageSound.play();
                }
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
        }
    }

    function appendMessage(message, color = colorGray, tabIdx = 0) {
        var model = cliModel.get(tabIdx)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message.linkify()}`, msgColor: color})
    }

    function appendPrivateMessage(tabIdx, message, from, color = colorCyan) {
        for(var i = 0; i < tabModel.count; i++) {
            var tab = tabModel.get(i)
            if(tabIdx === i) {
                for(var j = 0; j < cliModel.count; j++) {
                    var cli = cliModel.get(j)
                    if(cli.tabName.toLowerCase() === tab.title.toLowerCase()) {
                        if(from) {
                            cli.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${from}: ${message.linkify()}`, msgColor: color})
                        }
                        else {
                            cli.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message.linkify()}`, msgColor: color})
                        }
                    }
                }
            }
        }
    }

    function appendNote(message) {
        var model = cliModel.get(1)
        model.attributes.append({message:`[${TimestampUtils.currentTimestamp()}] ${message}`})
    }

    function clearMessages(tabIdx) {
        var model = cliModel.get(tabIdx)
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
        cliModel.append({tabName: callsign.toLowerCase(), tabId: idx, attributes: [], realName: false})
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

            NearbyAtc {
                id: nearbyAtc
                anchors.fill: parent
                enroute: nearbyEnroute
                approach: nearbyApproach
                tower: nearbyTower
                ground: nearbyGround
                delivery: nearbyDelivery
                atis: nearbyAtis
                onStartChatSession: function(callsign) {
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

                                for(var i = 0; i < cliModel.count; i++) {
                                    var cli = cliModel.get(i)
                                    cli.tabId = i
                                }
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
                    tabName: "messages"
                    tabId: 0 // messages
                    attributes: []
                }
                ListElement {
                    tabName: "notes"
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
                                            linkColor: '#ffffff'
                                            onLinkActivated: Qt.openUrlExternally(link)
                                            MouseArea {
                                                anchors.fill: parent
                                                acceptedButtons: Qt.NoButton
                                                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            }
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
                                visible: tabId === currentTab

                                property var commandHistory: []
                                property int historyIndex: -1
                                property string commandLineValue: ""

                                background: Rectangle {
                                    color: 'transparent'
                                    border.color: '#5C5C5C'
                                }

                                Keys.onPressed: function(event) {
                                    if(event.key === Qt.Key_Escape) {
                                        cliTextField.clear()
                                    }
                                    else if(event.key === Qt.Key_Down) {
                                        if(historyIndex == -1) {
                                            commandLineValue = cliTextField.text
                                        }
                                        historyIndex--
                                        if(historyIndex < 0) {
                                            historyIndex = -1
                                            cliTextField.text = commandLineValue
                                            return
                                        }
                                        cliTextField.text = commandHistory[historyIndex]
                                        return
                                    }
                                    else if(event.key === Qt.Key_Up) {
                                        if(historyIndex == -1) {
                                            commandLineValue = cliTextField.text
                                        }
                                        historyIndex++
                                        if(historyIndex >= commandHistory.length) {
                                            historyIndex = -1
                                            cliTextField.text = commandLineValue
                                            return
                                        }
                                        cliTextField.text = commandHistory[historyIndex]
                                        return
                                    }
                                    else if(event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                                        var cmd = cliTextField.text.split(" ").filter(function(i) { return i })

                                        commandHistory.unshift(cliTextField.text)
                                        historyIndex = -1

                                        try {

                                            if(cliTextField.text.startsWith(".simip")) {
                                                if(cmd.length > 2) {
                                                    throw `Too many parameters. Expected .simip IP`
                                                }
                                                if(cmd.length > 1) {
                                                    AppConfig.XplaneNetworkAddress = cmd[1]
                                                    AppConfig.saveConfig()
                                                    appendMessage(`X-Plane network address set to ${AppConfig.XplaneNetworkAddress}. You must restart xPilot for the changes to take effect.`, colorYellow, currentTab)
                                                }
                                                else {
                                                    AppConfig.XplaneNetworkAddress = "127.0.0.1"
                                                    AppConfig.saveConfig()
                                                    appendMessage("X-Plane network address reset to localhost (127.0.0.1). You must restart xPilot for the changes to take effect.", colorYellow, currentTab)
                                                }
                                                cliTextField.clear()
                                            }
                                            else if(cliTextField.text.startsWith(".visualip")) {
                                                if(cmd.length < 2) {
                                                    AppConfig.VisualMachines = []
                                                    AppConfig.saveConfig()
                                                    appendMessage("X-Plane visual machine addresses cleared. You must restart xPilot for the changes to take effect.", colorYellow, currentTab);
                                                }
                                                else {
                                                    AppConfig.VisualMachines = []
                                                    for(var x = 1; x < cmd.length; x++) {
                                                        AppConfig.VisualMachines.push(cmd[x])
                                                    }
                                                    AppConfig.saveConfig()
                                                    appendMessage(`X-Plane visual machine(s) set to ${AppConfig.VisualMachines.join(", ")}. You must restart xPilot for the changes to take effect.`, colorYellow, currentTab)
                                                }
                                                cliTextField.clear()
                                            }
                                            else if(cliTextField.text.startsWith(".downloadcsl")) {
                                                createCslDownloadWindow()
                                                cliTextField.clear()
                                            }
                                            else if(cliTextField.text.startsWith(".clear")) {
                                                clearMessages(currentTab)
                                                cliTextField.clear()
                                            }
                                            else if(cliTextField.text.startsWith(".copy")) {
                                                var text = []
                                                var model = cliModel.get(currentTab)
                                                for(var i = 0; i < model.attributes.rowCount(); i++) {
                                                    text.push(model.attributes.get(i).message)
                                                }
                                                clipboard.setText(text.join("\n"))
                                                appendMessage("Messages copied to clipboard.", colorYellow, currentTab)
                                                cliTextField.clear()
                                            }
                                            else {
                                                if(cliTextField.text.startsWith(".")) {
                                                    if(!simConnected) {
                                                        throw "X-Plane connection not found. Please make sure X-Plane is running and a flight is loaded."
                                                    }
                                                    switch(cmd[0].toLowerCase())
                                                    {
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
                                                        networkManager.sendWallop(cmd.slice(1).join(" "))
                                                        cliTextField.clear()
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
                                                        networkManager.requestControllerAtis(cmd[1])
                                                        cliTextField.clear()
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
                                                        if(cmd.length < 2) {
                                                            throw `Not enough parameters. Expected .tx com1 or .tx com2`
                                                        }
                                                        var radio = cmd[1].toLowerCase() === "com1" ? 1 : 2
                                                        xplaneAdapter.setAudioComSelection(radio)
                                                        cliTextField.clear()
                                                        break;
                                                    case ".rx":
                                                        if(cmd.length < 3) {
                                                            throw `Not enough parameters. Expected .rx com<n> on|off`
                                                        }
                                                        if(cmd[1].toLowerCase() !== "com1" && cmd[1].toLowerCase() !== "com2") {
                                                            throw `Invalid parameters. Expected .rx com<n> on|off`
                                                        }
                                                        if(cmd[2].toLowerCase() !== "on" && cmd[2].toLowerCase() !== "off") {
                                                            throw `Invalid parameters. Expected .rx com<n> on|off`
                                                        }
                                                        var radio = cmd[1].toLowerCase() === "com1" ? 1 : 2
                                                        var status = cmd[2].toLowerCase() === "on" ? 1 : 0
                                                        xplaneAdapter.setAudioSelection(radio, status)
                                                        cliTextField.clear()
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
                                                        if(!simConnected)
                                                        {
                                                            throw "X-Plane connection not found. Please make sure X-Plane is running and a flight is loaded."
                                                        }
                                                        if(networkConnected)
                                                        {
                                                            throw "You must first disconnect from the network before using TowerView"
                                                        }
                                                        var tvServerAddress = "localhost"
                                                        var tvCallsign = "TOWER"
                                                        if(cmd.length >= 2)
                                                        {
                                                            tvServerAddress = cmd[1]
                                                            if(cmd.length >= 3)
                                                            {
                                                                tvCallsign = cmd[2].toUpperCase()
                                                            }
                                                        }
                                                        networkManager.connectTowerView(tvCallsign, tvServerAddress)
                                                        cliTextField.clear()
                                                        break;
                                                    case ".ignore":
                                                        if(cmd.length < 2) {
                                                            throw `Not enough parameters. Expected .ignore CALLSIGN`
                                                        }
                                                        xplaneAdapter.ignoreAircraft(cmd[1])
                                                        cliTextField.clear()
                                                        break;
                                                    case ".ignorelist":
                                                        xplaneAdapter.showIgnoreList()
                                                        cliTextField.clear()
                                                        break;
                                                    case ".unignore":
                                                        if(cmd.length < 2) {
                                                            throw `Not enough parameters. Expected .unignore CALLSIGN`
                                                        }
                                                        xplaneAdapter.unignoreAircraft(cmd[1])
                                                        cliTextField.clear()
                                                        break;
                                                    default:
                                                        throw `Unknown command: ${cmd[0].toLowerCase()}`
                                                    }
                                                }
                                                else {
                                                    if(!cliTextField.text || /^\s*$/.test(cliTextField.text)) {
                                                        return; // skip empty message
                                                    }
                                                    if(currentTab == 0) {
                                                        if(!simConnected) {
                                                            throw "X-Plane connection not found. Please make sure X-Plane is running and a flight is loaded."
                                                        }
                                                        if(!networkConnected) {
                                                            throw "Not connected to network."
                                                        }
                                                        networkManager.sendRadioMessage(cliTextField.text)
                                                        appendMessage(cliTextField.text, colorCyan)
                                                        cliTextField.clear()
                                                    }
                                                    else if(currentTab == 1) {
                                                        // notes
                                                        appendNote(cliTextField.text)
                                                        cliTextField.clear()
                                                    }
                                                    else {
                                                        if(!simConnected) {
                                                            throw "X-Plane connection not found. Please make sure X-Plane is running and a flight is loaded."
                                                        }
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
                                            }
                                        }
                                        catch(err) {
                                            appendMessage(err, colorRed, currentTab)
                                            cliTextField.clear()
                                        }
                                    }
                                    historyIndex = -1
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

    function absoluteMousePos(mouseArea) {
        var windowAbs = mouseArea.mapToItem(null, mouseArea.mouseX, mouseArea.mouseY)
        return Qt.point(windowAbs.x + mainWindow.x, windowAbs.y + mainWindow.y)
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

        property var startMousePos;
        property var startWindowPos;
        property var startWindowSize;

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
            if (mainWindow.visibility == Window.Maximized || mainWindow.visibility == Window.FullScreen) {
                mainWindow.showNormal()
            } else {
                mainWindow.showFullScreen()
            }
            moveable = false
        }

        onPressed: function(mouse) {
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

            if (mainWindow.startSystemMove != undefined && Qt.platform.os !== "osx") {
                if (activeEdges == 0) {
                    mainWindow.startSystemMove()
                } else {
                    mainWindow.startSystemResize(activeEdges)
                }
            } else {
                lastMouseX = mouseX
                lastMouseY = mouseY
                moveable = (activeEdges === 0)
                startMousePos = absoluteMousePos(this)
                startWindowPos = Qt.point(mainWindow.x, mainWindow.y)
                startWindowSize = Qt.size(mainWindow.width, mainWindow.height)
            }
        }

        onReleased: {
            activeEdges = 0
            moveable = false
        }

        onMouseXChanged: {
            // Use system native move & resize on Qt >= 5.15
            if (mainWindow.startSystemMove != undefined && Qt.platform.os !== "osx") {
                return
            }

            if (mainWindow.visibility == Window.Maximized || mainWindow.visibility == Window.FullScreen || !pressed) {
                return
            }

            var abs;
            var newWidth;
            var newX;

            if(activeEdges & Qt.RightEdge) {
                abs = absoluteMousePos(this)
                newWidth = Math.max(mainWindow.minimumWidth, startWindowSize.width + (abs.x - startMousePos.x))
                mainWindow.setGeometry(mainWindow.x, mainWindow.y, newWidth, mainWindow.height)
            }
            else if(activeEdges & Qt.LeftEdge) {
                abs = absoluteMousePos(this)
                newWidth = Math.max(mainWindow.minimumWidth, startWindowSize.width - (abs.x - startMousePos.x))
                newX = startWindowPos.x - (newWidth - startWindowSize.width)
                mainWindow.setGeometry(newX, mainWindow.y, newWidth, mainWindow.height)
            }
            else if(moveable) {
                mainWindow.x += (mouseX - lastMouseX)
            }
        }

        onMouseYChanged: {
            // Use system native move & resize on Qt >= 5.15
            if (mainWindow.startSystemMove != undefined && Qt.platform.os !== "osx") {
                return
            }

            if (mainWindow.visibility == Window.Maximized || mainWindow.visibility == Window.FullScreen || !pressed) {
                return
            }

            var abs;
            var newHeight;
            var newY;

            if(activeEdges & Qt.TopEdge) {
                abs = absoluteMousePos(this)
                newHeight = Math.max(mainWindow.minimumHeight, startWindowSize.height - (abs.y - startMousePos.y))
                newY = startWindowPos.y - (newHeight - startWindowSize.height)
                mainWindow.setGeometry(mainWindow.x, newY, mainWindow.width, newHeight)
            }
            else if(activeEdges & Qt.BottomEdge) {
                abs = absoluteMousePos(this)
                newHeight = Math.max(mainWindow.minimumHeight, startWindowSize.height + (abs.y - startMousePos.y))
                mainWindow.setGeometry(mainWindow.x, mainWindow.y, mainWindow.width, newHeight)
            }
            else if(moveable) {
                mainWindow.y += (mouseY - lastMouseY)
            }
        }
    }

    function createCslDownloadWindow() {
        var comp = Qt.createComponent("qrc:/Resources/Components/DownloadCSLModels/DownloadModels.qml")
        if(comp.status === Component.Ready) {
            downloadCslWindow = comp.createObject(mainWindow)
            downloadCslWindow.open()
        }
    }

    function createSetXplanePathWindow() {
        var comp = Qt.createComponent("qrc:/Resources/Components/DownloadCSLModels/SetXplanePath.qml")
        if(comp.status === Component.Ready) {
            setXplanePathWindow = comp.createObject(mainWindow)
            setXplanePathWindow.open()
        }
    }

    function createExtractCslModelsWindow() {
        var comp = Qt.createComponent("qrc:/Resources/Components/DownloadCSLModels/ExtractModels.qml")
        if(comp.status === Component.Ready) {
            extractCslModelsWindow = comp.createObject(mainWindow)
            extractCslModelsWindow.open()
        }
    }
}
