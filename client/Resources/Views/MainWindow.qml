import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import QtQuick.Layouts
import QtMultimedia

import org.vatsim.xpilot

import "../Scripts/FrequencyUtils.js" as FrequencyUtils
import "../Scripts/TimestampUtils.js" as TimestampUtils
import "../Scripts/StringUtils.js" as StringUtils
import "../Ui/Colors.js" as Colors
import "../Controls"
import "../Components"
import "../Components/DownloadCSLModels"
import "../Components/VersionCheck"

Window {
    id: mainWindow
    title: "xPilot"
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "#272C2E"
    minimumHeight: 250
    minimumWidth: 840
    width: minimumWidth
    height: minimumHeight

    // CSL installation
    property QtObject downloadCslWindow
    property QtObject setXplanePathWindow
    property QtObject extractCslModelsWindow

    property int activeMessageTab
    property string networkCallsign
    property var radioStackState
    property bool networkConnected: false
    property bool initialized: false
    property bool simConnected: false

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
        id: newVersionAvailableDialog
    }

    DownloadingUpdate {
        id: downloadingUpdateDialog
    }

    DisconnectDialog {
        id: confirmClose
        onExitApplication: Qt.exit(0)
    }

    MicrophoneCalibrationRequired {
        id: micCalibrationRequiredDialog
    }

    ConfigRequiredDialog {
        id: configRequiredDialog
    }

    SettingsWindow {
        id: settingsWindowDialog
    }

    ConnectWindow {
        id: connectWindowDialog
    }

    Connections {
        target: AppConfig

        function onPermissionError(error) {
            appendMessage(`Configuration Error ${error}`, Enum.MessageType.Error)
            errorSound.play()
        }

        function onSettingsChanged() {
            if (AppConfig.KeepWindowVisible) {
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

        appendMessage(`Welcome to xPilot v${appVersion}`, Enum.MessageType.Info)
        if (AppConfig.XplaneNetworkAddress !== "127.0.0.1" && AppConfig.XplaneNetworkAddress !== "localhost") {
            appendMessage(`Waiting for X-Plane connection (${AppConfig.XplaneNetworkAddress})...
                          Please make sure X-Plane is running and a flight is loaded.`, Enum.MessageType.Info)
        } else {
            appendMessage("Waiting for X-Plane connection... Please make sure X-Plane is running and a flight is loaded.", Enum.MessageType.Info)
        }

        if (AppConfig.configRequired()) {
            configRequiredDialog.open()
        }
        else if (!AppConfig.SilenceModelInstall) {
            createCslDownloadWindow()
        }

        if (AppConfig.KeepWindowVisible) {
            mainWindow.flags |= Qt.WindowStaysOnTopHint
        }

        if (AppConfig.WindowConfig.Maximized) {
            mainWindow.showFullScreen()
        }
        else {
            width = Math.max(minimumWidth, AppConfig.WindowConfig.Width)
            height = Math.max(minimumHeight, AppConfig.WindowConfig.Height)
            x = AppConfig.WindowConfig.X
            y = AppConfig.WindowConfig.Y
        }
        initialized = true
    }

    onClosing: function(close) {
        close.accepted = !networkConnected
        onTriggered: if (networkConnected) confirmClose.open()
    }

    onXChanged: {
        if (initialized) {
            if (visibility === Window.Maximized || visibility === Window.FullScreen) {
                return
            }
            AppConfig.WindowConfig.X = x
            AppConfig.saveConfig()
        }
    }

    onYChanged: {
        if (initialized) {
            if (visibility === Window.Maximized || visibility === Window.FullScreen) {
                return
            }
            AppConfig.WindowConfig.Y = y
            AppConfig.saveConfig()
        }
    }

    onVisibilityChanged: function(visibility) {
        if (initialized && visibility !== Window.Hidden) {
            var isMaximized = (visibility === Window.Maximized || visibility === Window.FullScreen)
            if (AppConfig.WindowConfig.Maximized && !isMaximized) {
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
        if (visibility === Window.Maximized || visibility === Window.FullScreen) {
            return
        }

        if (initialized) {
            AppConfig.WindowConfig.Height = height
            AppConfig.saveConfig()
        }
    }

    onWidthChanged: {
        if (visibility === Window.Maximized || visibility === Window.FullScreen) {
            return
        }

        if (initialized) {
            AppConfig.WindowConfig.Width = width
            AppConfig.saveConfig()
        }
    }

    Connections {
        target: versionCheck

        function onNewVersionAvailable() {
            newVersionAvailableDialog.open()
        }

        function onDownloadStarted() {
            downloadingUpdateDialog.open()
        }

        function onNoUpdatesAvailable() {
            appendMessage("Version check complete. You are running the latest version of xPilot.", Enum.MessageType.Info)
        }

        function onErrorEncountered(error) {
            appendMessage(error, Enum.MessageType.Error)
            errorSound.play()
        }
    }

    Connections {
        target: typeCodeDatabase

        function onTypeCodeDownloadError(error) {
            appendMessage(error, Enum.MessageType.Error)
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
            appendMessage(`CSL aircraft model package successfully installed! Please restart xPilot and X-Plane before connecting to the network.
                          If you need to install the model set for another X-Plane installation, enter the command .downloadcsl`, Enum.MessageType.Info)
            AppConfig.SilenceModelInstall = true
        }

        function onDownloadProgressChanged(val) {
            downloadCslWindow.pctProgress = val
        }

        function onUnzipProgressChanged(val) {
            extractCslModelsWindow.pctProgress = val
        }

        function onErrorEncountered(error) {
            if (downloadCslWindow != null) {
                downloadCslWindow.destroy()
            }
            if (setXplanePathWindow != null) {
                setXplanePathWindow.destroy()
            }
            if (extractCslModelsWindow != null) {
                extractCslModelsWindow.destroy()
            }
            appendMessage(error, Enum.MessageType.Error)
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
        target: serverListManager

        function onServerListDownloaded(count) {
            appendMessage(`Server list download succeeded. ${count} servers found.`, Enum.MessageType.Info)
        }

        function onServerListDownloadError(count) {
            appendMessage("Server list download failed. Using previously-cached server list.", Enum.MessageType.Error)
        }
    }

    Connections {
        target: xplaneAdapter

        function onNngSocketError(error) {
            appendMessage(`Socket Error: ${error}`, Enum.MessageType.Error)
            errorSound.play()
        }

        function onRadioStackStateChanged(stack) {
            if (radioStackState !== stack) {
                radioStackState = stack
            }
        }

        function onSimConnectionStateChanged(state) {
            if (!simConnected && state) {
                appendMessage("X-Plane connection established.", Enum.MessageType.Info)
            }
            else if (simConnected && !state) {
                networkManager.disconnectFromNetwork()
                appendMessage("X-Plane connection lost. Please make sure X-Plane is running and a flight is loaded.", Enum.MessageType.Error)
                mainWindow.alert(0)
                errorSound.play()
            }
            simConnected = state
        }

        function onReplayModeDetected() {
            if (networkConnected) {
                networkManager.disconnectFromNetwork()
                appendMessage("You have been disconnected from the network because Replay Mode is enabled.", Enum.MessageType.Error)
                if (AppConfig.AlertDisconnect) {
                    mainWindow.alert(0)
                    errorSound.play()
                }
            }
        }

        function onInvalidPluginVersion() {
            appendMessage(`Unsupported xPilot plugin version detected.
                          Please close X-Plane and reinstall the latest version of xPilot.`, Enum.MessageType.Error)
            errorSound.play()
            mainWindow.alert(0)
        }

        function onInvalidCslConfiguration() {
            appendMessage(`No valid CSL paths are configured or enabled, or you have no CSL models installed.
                          Please verify the CSL configuration in X-Plane (Plugins > xPilot > Settings). If you need assistance configuring your CSL paths,
                          see the "CSL Configuration" section in the xPilot Documentation (http://xpilot-project.org).
                          Restart X-Plane and xPilot after you have properly configured your CSL models.
                          You can have xPilot install a model set for you by entering the command .downloadcsl`, Enum.MessageType.Error)
            errorSound.play()
            mainWindow.alert(0)
        }

        function onRadioMessageSent(message) {
            appendMessage(message, Enum.MessageType.OutgoingRadio)
        }

        function onAircraftIgnored(callsign) {
            appendMessage(`${callsign} has been added to the ignore list.`, Enum.MessageType.Info)
        }

        function onAircraftAlreadyIgnored(callsign) {
            appendMessage(`${callsign} is already in the ignore list.`, Enum.MessageType.Error)
            errorSound.play()
        }

        function onAircraftUnignored(callsign) {
            appendMessage(`${callsign} has been removed from the ignore list.`, Enum.MessageType.Info)
        }

        function onAircraftNotIgnored(callsign) {
            appendMessage(`${callsign} was not found in the ignore list.`, Enum.MessageType.Error)
            errorSound.play()
        }

        function onIgnoreList(list) {
            if (list.length > 0) {
                appendMessage(`The following aircraft are currently ignored: ${list.join(", ")}`, Enum.MessageType.Info)
            }
            else {
                appendMessage("The ignore list is currently empty.", Enum.MessageType.Info)
            }
        }
    }

    Connections {
        target: audio

        function onNotificationPosted(message, type) {
            appendMessage(message, type)
        }
    }

    Connections {
        target: networkManager

        function onMicrophoneCalibrationRequired() {
            micCalibrationRequiredDialog.open()
        }

        function onNetworkConnected(callsign) {
            networkCallsign = callsign
            networkConnected = true
        }

        function onNetworkDisconnected(forced) {
            networkConnected = false
            nearbyEnroute.clear()
            nearbyApproach.clear()
            nearbyTower.clear()
            nearbyGround.clear()
            nearbyDelivery.clear()
            nearbyAtis.clear()

            if (forced && AppConfig.AlertDisconnect) {
                errorSound.play()
                mainWindow.alert(0)
            }
        }

        function onNotificationPosted(message, type) {
            appendMessage(message, type)
        }

        function onServerMessageReceived(message) {
            if (message.includes("donate.vatsim.net")) {
                appendMessage(`SERVER: ${message}`, Colors.Magenta)
            } else {
                appendMessage(`SERVER: ${message}`, Enum.MessageType.Server)
            }
        }

        function onBroadcastMessageReceived(from, message) {
            appendMessage(`[BROADCAST] ${from}: ${message}`, Enum.MessageType.Broadcast)
            if (AppConfig.AlertNetworkBroadcast) {
                broadcastSound.play()
                mainWindow.alert(0)
            }
        }

        function onWallopSent(message) {
            appendMessage(`[WALLOP] ${message}`, Enum.MessageType.Wallop)
        }

        function onSelcalAlertReceived(from, frequencies) {
            appendMessage(`SELCAL alert received on ${FrequencyUtils.fromNetworkFormat(frequencies[0])}`, Enum.MessageType.Info)
            if (AppConfig.AlertSelcal) {
                mainWindow.alert(0)
                if (!radioStackState.SelcalMuteOverride) {
                    selcalSound.play()
                }
            }
        }

        function onRealNameReceived(callsign, name) {
            var tabIdx = getChatTabIndex(callsign)
            if (tabIdx !== undefined) {
                var model = tabModel.get(tabIdx)
                if(!model.realNameReceived) {
                    model.messages.insert(0, { message: `<font style="color:${Colors.Yellow}">Name: ${name}</font>` })
                    model.realNameReceived = true
                }
            }
        }

        function onPrivateMessageReceived(from, message) {
            var tabIdx = getChatTabIndex(from)
            if(tabIdx === undefined) {
                createChatTab(from)
                tabIdx = getChatTabIndex(from)
                appendMessage(`${from}: ${message}`, Enum.MessageType.IncomingPrivate, tabIdx)
                if(AppConfig.AlertPrivateMessage) {
                    mainWindow.alert(0)
                    newMessageSound.play()
                }
            } else {
                appendMessage(`${from}: ${message}`, Enum.MessageType.IncomingPrivate, tabIdx)
                if(AppConfig.AlertPrivateMessage) {
                    mainWindow.alert(0)
                    privateMessageSound.play()
                }
            }
        }

        function onPrivateMessageSent(to, message) {
            var tabIdx = getChatTabIndex(to)
            if(tabIdx === undefined) {
                createChatTab(to)
                tabIdx = getChatTabIndex(to)
            }
            appendMessage(message, Enum.MessageType.OutgoingPrivate, tabIdx)
        }

        function onRadioMessageReceived(args) {
            var message = ""
            if (args.DualReceiver) {
                var freqString = ""
                if (args.Frequencies.length > 1) {
                    freqString = `${FrequencyUtils.fromNetworkFormat(args.Frequencies[0])} & ${FrequencyUtils.fromNetworkFormat(args.Frequencies[1])}`;
                }
                else {
                    freqString = FrequencyUtils.fromNetworkFormat(args.Frequencies[0])
                }
                message = `${args.From} on ${freqString}: ${args.Message}`
            }
            else {
                message = `${args.From}: ${args.Message}`
            }

            appendMessage(message, args.IsDirect ? Enum.MessageType.IncomingRadioPrimary : Enum.MessageType.IncomingRadioSecondary)

            if (args.IsDirect) {
                if (AppConfig.AlertDirectRadioMessage) {
                    mainWindow.alert(0)
                    directRadioMessageSound.play()
                }
            }
            else {
                if (AppConfig.AlertRadioMessage) {
                    radioMessageSound.play()
                }
            }
        }

        function onControllerAtisReceived(callsign, atis) {
            appendMessage(`${callsign} ATIS:`, Colors.Orange)
            atis.forEach(function (line) {
                appendMessage(line, Colors.Orange)
            })
        }

        function onMetarReceived(station, metar) {
            appendMessage(`METAR: ${metar}`, Colors.Orange)
        }
    }

    Connections {
        target: controllerManager

        function findController(controllers, callsign) {
            for (var i = 0; i < controllers.count; i++) {
                var controller = controllers.get(i)
                if (callsign === controller.Callsign) {
                    return i
                }
            }
            return undefined
        }

        function onControllerAdded(controller) {
            var idx
            if (controller.Callsign.endsWith("_CTR") || controller.Callsign.endsWith("_FSS")) {
                idx = findController(nearbyEnroute, controller.Callsign)
                if (idx === undefined) {
                    nearbyEnroute.append(controller)
                }
            }
            else if (controller.Callsign.endsWith("_APP") || controller.Callsign.endsWith("_DEP")) {
                idx = findController(nearbyApproach, controller.Callsign)
                if (idx === undefined) {
                    nearbyApproach.append(controller)
                }
            }
            else if (controller.Callsign.endsWith("_TWR")) {
                idx = findController(nearbyTower, controller.Callsign)
                if (idx === undefined) {
                    nearbyTower.append(controller)
                }
            }
            else if (controller.Callsign.endsWith("_GND")) {
                idx = findController(nearbyGround, controller.Callsign)
                if (idx === undefined) {
                    nearbyGround.append(controller)
                }
            }
            else if (controller.Callsign.endsWith("_DEL")) {
                idx = findController(nearbyDelivery, controller.Callsign)
                if (idx === undefined) {
                    nearbyDelivery.append(controller)
                }
            }
            else if (controller.Callsign.endsWith("_ATIS")) {
                idx = findController(nearbyAtis, controller.Callsign)
                if (idx === undefined) {
                    nearbyAtis.append(controller)
                }
            }
        }

        function onControllerDeleted(controller) {
            var idx;
            if (controller.Callsign.endsWith("_CTR") || controller.Callsign.endsWith("_FSS")) {
                idx = findController(nearbyEnroute, controller.Callsign)
                if (idx !== undefined) {
                    nearbyEnroute.remove(idx)
                }
            }
            else if (controller.Callsign.endsWith("_APP") || controller.Callsign.endsWith("_DEP")) {
                idx = findController(nearbyApproach, controller.Callsign)
                if (idx !== undefined) {
                    nearbyApproach.remove(idx)
                }
            }
            else if (controller.Callsign.endsWith("_TWR")) {
                idx = findController(nearbyTower, controller.Callsign)
                if (idx !== undefined) {
                    nearbyTower.remove(idx)
                }
            }
            else if (controller.Callsign.endsWith("_GND")) {
                idx = findController(nearbyGround, controller.Callsign)
                if (idx !== undefined) {
                    nearbyGround.remove(idx)
                }
            }
            else if (controller.Callsign.endsWith("_DEL")) {
                idx = findController(nearbyDelivery, controller.Callsign)
                if (idx !== undefined) {
                    nearbyDelivery.remove(idx)
                }
            }
            else if (controller.Callsign.endsWith("_ATIS")) {
                idx = findController(nearbyAtis, controller.Callsign)
                if (idx !== undefined) {
                    nearbyAtis.remove(idx)
                }
            }
        }
    }

    function appendMessage(message, type = Enum.MessageType.Info, tabIdx = 0) {
        var textColor = Colors.Yellow
        switch (type) {
            case Enum.MessageType.Server:
                textColor = Colors.Green
                break
            case Enum.MessageType.IncomingPrivate:
                textColor = Colors.Cyan
                break
            case Enum.MessageType.OutgoingPrivate:
                textColor = Colors.LightGray
                break
            case Enum.MessageType.TextOverride:
                textColor = Colors.IndianRed
                break
            case Enum.MessageType.IncomingRadioPrimary:
                textColor = Colors.White
                break
            case Enum.MessageType.IncomingRadioSecondary:
                textColor = Colors.Gray
                break
            case Enum.MessageType.OutgoingRadio:
                textColor = Colors.Cyan
                break
            case Enum.MessageType.Broadcast:
                textColor = Colors.Orange
                break
            case Enum.MessageType.Wallop:
                textColor = Colors.Red
                break
            case Enum.MessageType.Info:
                textColor = Colors.Yellow
                break
            case Enum.MessageType.Error:
                textColor = Colors.Red
                break
            default:
                textColor = type
        }
        var model = tabModel.get(tabIdx)
        if (model) {
            model.messages.append({ message: `<font color="${textColor}">[${TimestampUtils.currentTimestamp()}] ${message.linkify()}</font>` })
            if(tabControl.currentTabIndex !== tabIdx) {
                model.hasMessageWaiting = true
            }
        }
    }

    function getChatTabIndex(callsign) {
        for (var i = 2; i < tabModel.count; i++) {
            if (tabModel.get(i).title.toUpperCase() === callsign.toUpperCase()) {
                return i
            }
        }
        return undefined
    }

    function createChatTab(callsign) {
        tabModel.append({ title: callsign.toUpperCase(), isCloseable: true, messages: [], hasMessageWaiting: false, realNameReceived: false })
        tabControl.currentTabIndex = tabModel.count - 1
        tabControl.callsign = callsign.toUpperCase()
        networkManager.requestRealName(callsign)
    }

    function focusOrCreateTab(callsign) {
        var tabIdx = getChatTabIndex(callsign)
        if (tabIdx === undefined) {
            createChatTab(callsign)
        } else {
            tabControl.currentTabIndex = tabIdx
            tabControl.callsign = callsign
        }
    }

    Rectangle {
        id: windowFrame
        anchors.fill: parent
        color: "transparent"
        border.color: Qt.platform.os === "osx" ? "transparent" : "#000000"
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

        ListModel {
            id: tabModel
            ListElement{
                title: "Messages"
                isCloseable: false
                messages: []
                hasMessageWaiting: false
            }
            ListElement {
                title: "Notes"
                isCloseable: false
                messages: []
                hasMessageWaiting: false
            }
        }

        Rectangle {
            id: msgControl
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 1
            color: "#141618"

            TabControl {
                id: tabControl
                model: tabModel
                anchors.fill: parent
                anchors.margins: 10

                onCommandSubmitted: function(message, tabIndex) {

                    var model = tabModel.get(tabIndex)

                    if(message.startsWith(".clear")) {
                        if(model) {
                            model.messages.clear()
                        }
                        return
                    }
                    else if(message.startsWith(".copy")) {
                        var temp = []
                        for(var i = 0; i < model.messages.count; i++) {
                            temp.push(model.messages.get(i).message)
                        }
                        clipboard.setText(temp.join("\n"))
                        appendMessage("Messages copied to clipboard.", Enum.MessageType.Info, tabIndex)
                        return
                    }

                    if(tabIndex === 0) {
                        try {
                            var cmd = message.split(" ").filter(function(i){return i})

                            if(message.startsWith(".simip")) {
                                if(cmd.length > 2) {
                                    throw "Too many parameters. Expected .simip IP"
                                }
                                if(cmd.length > 1) {
                                    AppConfig.XplaneNetworkAddress = cmd[1]
                                    AppConfig.saveConfig()
                                    appendMessage(`X-Plane network address set to ${AppConfig.XplaneNetworkAddress}.
                                                  You must restart xPilot for the changes to take effect.`, Enum.MessageType.Info, tabIndex)
                                }
                                else {
                                    AppConfig.XplaneNetworkAddress = "127.0.0.1"
                                    AppConfig.saveConfig()
                                    appendMessage(`X-Plane network address reset to localhost (127.0.0.1).
                                                  You must restart xPilot for the changes to take effect.`, Enum.MessageType.Info, tabIndex)
                                }
                            }
                            else if(message.startsWith(".visualip")) {
                                if(cmd.length < 2) {
                                    AppConfig.VisualMachines = []
                                    AppConfig.saveConfig()
                                    appendMessage(`X-Plane visual machine addresses cleared.
                                                  You must restart xPilot for the changes to take effect.`, Enum.MessageType.Info, tabIndex)
                                }
                                else {
                                    AppConfig.VisualMachines = []
                                    for(var vm = 1; vm < cmd.length; vm++) {
                                        AppConfig.VisualMachines.push(cmd[vm])
                                    }
                                    AppConfig.saveConfig()
                                    appendMessage(`X-Plane visual machine(s) set to ${AppConfig.VisualMachines.join(", ")}.
                                                  You must restart xPilot for the changes to take effect.`, Enum.MessageType.Info, tabIndex)
                                }
                            }
                            else if(message === ".downloadcsl") {
                                if(AppConfig.configRequired()) {
                                    appendMessage(`Before you can install the CSL model set, you must configure xPilot.
                                                  Open the Settings window to configure xPilot.`, Enum.MessageType.Error, tabIndex)
                                }
                                else {
                                    createCslDownloadWindow()
                                }
                            }
                            else if(message === ".appdata") {
                                AppConfig.openAppDataFolder()
                            }
                            else {
                                if(message.startsWith(".")) {
                                    if(!simConnected) {
                                        throw "X-Plane connection not found. Please make sure X-Plane is running and a flight is loaded."
                                    }
                                    switch(cmd[0].toLowerCase()) {
                                    case ".chat":
                                        if(cmd.length < 2) {
                                            throw "Not enough parameters. Expected .chat CALLSIGN"
                                        }
                                        if(cmd[1].length > 12) {
                                            throw "Callsign too long."
                                        }
                                        focusOrCreateTab(cmd[1])
                                        break
                                    case ".msg":
                                        if(cmd.length < 3) {
                                            throw "Not enough parameters. Expected .msg CALLSIGN MESSAGE"
                                        }
                                        if(!networkConnected) {
                                            throw "Not connected to network."
                                        }
                                        if(cmd[1].length > 12) {
                                            throw "Callsign too long."
                                        }
                                        focusOrCreateTab(cmd[1])
                                        networkManager.sendPrivateMessage(cmd[1], cmd.slice(2).join(" "))
                                        break
                                    case ".wallop":
                                        if(cmd.length < 2) {
                                            throw "Not enough parameters. Expected .wallop MESSAGE"
                                        }
                                        if(!networkConnected) {
                                            throw "Not connected to network."
                                        }
                                        networkManager.sendWallop(cmd.slice(1).join(" "))
                                        break
                                    case ".wx":
                                    case ".metar":
                                        if(cmd.length < 2) {
                                            throw `Not enough parameters. Expected ${cmd[0]} STATION`
                                        }
                                        if(!networkConnected) {
                                            throw "Not connected to network."
                                        }
                                        networkManager.requestMetar(cmd[1])
                                        break
                                    case ".atis":
                                        if(cmd.length < 2) {
                                            throw "Not enough parameters. Expected .atis CALLSIGN"
                                        }
                                        if(!networkConnected) {
                                            throw "Not connected to network."
                                        }
                                        networkManager.requestControllerAtis(cmd[1])
                                        break
                                    case ".com1":
                                    case ".com2":
                                        if(cmd.length < 2) {
                                            throw `Not enough parameters. Expected ${cmd[0]} FREQUENCY`
                                        }
                                        if (!/^1\d\d[\.\,]\d{1,3}$/.test(cmd[1])) {
                                            throw "Invalid frequency format."
                                        }
                                        var frequency = FrequencyUtils.frequencyToInt(cmd[1])
                                        var radio = cmd[0].toLowerCase() === ".com1" ? 1 : 2
                                        if(radio === 1) {
                                            xplaneAdapter.setCom1Frequency(frequency)
                                        } else {
                                            xplaneAdapter.setCom2Frequency(frequency)
                                        }
                                        break
                                    case ".tx":
                                        if(cmd.length < 2) {
                                            throw "Not enough parameters. Expected .tx com1|com2"
                                        }
                                        if(cmd[1].toLowerCase() !== "com1" && cmd[1].toLowerCase() !== "com2") {
                                            throw "Invalid parameters. Expected .tx com1|com2"
                                        }
                                        var radio = cmd[1].toLowerCase() === "com1" ? 1 : 2
                                        xplaneAdapter.setAudioComSelection(radio)
                                        break
                                    case ".rx":
                                        if(cmd.length < 3) {
                                            throw `Not enough parameters. Expected .rx com1|com2 on|off`
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
                                        break
                                    case ".x":
                                    case ".xpndr":
                                    case ".xpdr":
                                    case ".squawk":
                                    case ".sq":
                                        if(cmd.length < 2) {
                                            throw `Not enough parameters. Expected ${cmd[0]} CODE`
                                        }
                                        if (!/^[0-7]{4}$/.test(cmd[1])) {
                                            throw "Invalid transponder code format."
                                        }
                                        var code = parseInt(cmd[1])
                                        xplaneAdapter.setTransponderCode(code)
                                        appendMessage(`Transponder code set to ${cmd[1]}.`, Enum.MessageType.Info)
                                        break
                                    case ".towerview":
                                        if(networkConnected) {
                                            throw "You must first disconnect from the network before using TowerView."
                                        }
                                        var tvServerAddress = "localhost"
                                        var tvCallsign = "TOWER"
                                        if(cmd.length >= 2) {
                                            tvServerAddress = cmd[1]
                                            if(cmd.length >= 3) {
                                                tvCallsign = cmd[2].toUpperCase()
                                            }
                                        }
                                        networkManager.connectTowerView(tvCallsign, tvServerAddress)
                                        break
                                    case ".ignore":
                                        if(cmd.length < 2) {
                                            throw "Not enough parameters. Expected .ignore CALLSIGN"
                                        }
                                        xplaneAdapter.ignoreAircraft(cmd[1])
                                        break
                                    case ".ignorelist":
                                        xplaneAdapter.showIgnoreList()
                                        break
                                    case ".unignore":
                                        if(cmd.length < 2) {
                                            throw "Not enough parameters. Expected .unignore CALLSIGN"
                                        }
                                        xplaneAdapter.unignoreAircraft(cmd[1])
                                        break
                                    default:
                                        throw `Unknown command: ${cmd[0].toLowerCase()}`
                                    }
                                }
                                else {
                                    if(!networkConnected) {
                                        throw "Not connected to network."
                                    }
                                    networkManager.sendRadioMessage(message)
                                    appendMessage(message, Enum.MessageType.OutgoingRadio)
                                }
                            }
                        }
                        catch(err) {
                            appendMessage(err, Enum.MessageType.Error, tabIndex)
                        }
                    }
                    else if(tabIndex === 1) {
                        appendMessage(message, Enum.MessageType.Info, tabIndex)
                    }
                    else {
                        if(message === ".close") {
                            tabModel.remove(tabIndex)
                            tabControl.currentTabIndex = 0
                        }
                        else {
                            if(!networkConnected) {
                                appendMessage("Not connected to network.", Enum.MessageType.Error, tabIndex)
                            }
                            networkManager.sendPrivateMessage(tabControl.callsign, message)
                        }
                    }
                }
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

        property var startMousePos
        property var startWindowPos
        property var startWindowSize

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

            var abs
            var newWidth
            var newX

            if (activeEdges & Qt.RightEdge) {
                abs = absoluteMousePos(this)
                newWidth = Math.max(mainWindow.minimumWidth, startWindowSize.width + (abs.x - startMousePos.x))
                mainWindow.setGeometry(mainWindow.x, mainWindow.y, newWidth, mainWindow.height)
            }
            else if (activeEdges & Qt.LeftEdge) {
                abs = absoluteMousePos(this)
                newWidth = Math.max(mainWindow.minimumWidth, startWindowSize.width - (abs.x - startMousePos.x))
                newX = startWindowPos.x - (newWidth - startWindowSize.width)
                mainWindow.setGeometry(newX, mainWindow.y, newWidth, mainWindow.height)
            }
            else if (moveable) {
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

            var abs
            var newHeight
            var newY

            if (activeEdges & Qt.TopEdge) {
                abs = absoluteMousePos(this)
                newHeight = Math.max(mainWindow.minimumHeight, startWindowSize.height - (abs.y - startMousePos.y))
                newY = startWindowPos.y - (newHeight - startWindowSize.height)
                mainWindow.setGeometry(mainWindow.x, newY, mainWindow.width, newHeight)
            }
            else if (activeEdges & Qt.BottomEdge) {
                abs = absoluteMousePos(this)
                newHeight = Math.max(mainWindow.minimumHeight, startWindowSize.height + (abs.y - startMousePos.y))
                mainWindow.setGeometry(mainWindow.x, mainWindow.y, mainWindow.width, newHeight)
            }
            else if (moveable) {
                mainWindow.y += (mouseY - lastMouseY)
            }
        }

        function absoluteMousePos(mouseArea) {
            var windowAbs = mouseArea.mapToItem(null, mouseArea.mouseX, mouseArea.mouseY)
            return Qt.point(windowAbs.x + mainWindow.x, windowAbs.y + mainWindow.y)
        }
    }

    function createCslDownloadWindow() {
        var comp = Qt.createComponent("qrc:/Resources/Components/DownloadCSLModels/DownloadModels.qml")
        if (comp.status === Component.Ready) {
            downloadCslWindow = comp.createObject(mainWindow)
            downloadCslWindow.open()
        }
    }

    function createSetXplanePathWindow() {
        var comp = Qt.createComponent("qrc:/Resources/Components/DownloadCSLModels/SetXplanePath.qml")
        if (comp.status === Component.Ready) {
            setXplanePathWindow = comp.createObject(mainWindow)
            setXplanePathWindow.open()
        }
    }

    function createExtractCslModelsWindow() {
        var comp = Qt.createComponent("qrc:/Resources/Components/DownloadCSLModels/ExtractModels.qml")
        if (comp.status === Component.Ready) {
            extractCslModelsWindow = comp.createObject(mainWindow)
            extractCslModelsWindow.open()
        }
    }
}
