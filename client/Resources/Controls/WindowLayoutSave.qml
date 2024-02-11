import QtQuick
import QtQuick.Window

import org.vatsim.xpilot

Item {
    id: root

    property QtObject window: Window.window
    property bool initialized: false

    Connections {
        target: root.window
        function onXChanged() { saveSettings() }
        function onYChanged() { saveSettings() }
        function onWidthChanged() { saveSettings() }
        function onHeightChanged() { saveSettings() }
        function onVisibilityChanged() { saveSettings() }
        function onScreenChanged() { saveSettings() }
    }

    function centerDefaultWindow() {
        AppConfig.WindowConfig.X = Screen.width / 2 - window.width / 2
        AppConfig.WindowConfig.Y = Screen.height / 2 - window.height / 2
        AppConfig.WindowConfig.Maximized = false
    }

    function restoreSettings() {
        if (AppConfig.WindowConfig.Width > Screen.desktopAvailableWidth) {
            AppConfig.WindowConfig.Width = Screen.desktopAvailableWidth - AppConfig.WindowConfig.X
            appendMessage(`AppConfig.WindowConfig.Width > Screen.desktopAvailableWidth: ${JSON.stringify(Screen)}`,
                          Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))
        }
        if (AppConfig.WindowConfig.Height > Screen.desktopAvailableHeight) {
            AppConfig.WindowConfig.Height = Screen.desktopAvailableHeight - AppConfig.WindowConfig.Y
            appendMessage(`AppConfig.WindowConfig.Height > Screen.desktopAvailableHeight: ${JSON.stringify(Screen)}`,
                          Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))
        }

        // Apply window geometry
        if(AppConfig.WindowConfig.Width && AppConfig.WindowConfig.Height && !initialized) {
            window.x = AppConfig.WindowConfig.X
            window.y = AppConfig.WindowConfig.Y
            window.width = AppConfig.WindowConfig.Width
            window.height = AppConfig.WindowConfig.Height
            window.visibility = AppConfig.WindowConfig.Maximized ? Window.FullScreen : Window.Windowed
            initialized = true

            var geometryJson = {x:window.x,y:window.y,width:window.width,height:window.height,visibility:window.visibility}
            appendMessage(`Applying window geometry: ${JSON.stringify(geometryJson)}`, Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))
        }

        appendMessage(`Available screens: ${JSON.stringify(Qt.application.screens)}`, Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))

        let screenFound = false
        for(let i = 0; i < Qt.application.screens.length; i++) {
            const screen = Qt.application.screens[i]
            if(screen.name === AppConfig.WindowConfig.ScreenName) {
                window.screen = screen
                screenFound = true
                appendMessage(`Found screen: ${JSON.stringify(window.screen)}`, Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))
                break
            }
        }

        if(!screenFound) {
            appendMessage(`Screen not found. Centering windwo on default screen.`, Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))
            centerDefaultWindow()
        }

        let offset = 100
        let isXinvalid = (window.x + window.width < window.screen.virtualX + offset) || (window.x > window.screen.virtualX + window.screen.width - offset)
        let isYinvalid = (window.y < window.screen.virtualY) || (window.y > window.screen.virtualY + window.screen.height - offset)

        if(isXinvalid || isYinvalid) {
            // center on active screen
            let screenX = (window.screen.virtualX + window.screen.width / 2) - (window.width / 2)
            let screenY = (window.screen.virtualY + window.screen.height / 2) - (window.height / 2)
            window.x = screenX
            window.y = screenY
            window.visibility = Window.Windowed
            appendMessage(`isXinvalid ${isXinvalid}, isYinvalid ${isYinvalid}`, Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))
        }
    }

    function saveSettings() {
        if(!initialized || !window.visible) return
        AppConfig.WindowConfig.X = window.x
        AppConfig.WindowConfig.Y = window.y
        AppConfig.WindowConfig.Width = window.width
        AppConfig.WindowConfig.Height = window.height
        AppConfig.WindowConfig.Maximized = (window.visibility === Window.FullScreen || window.visibility === Window.Maximized)
        AppConfig.WindowConfig.ScreenName = window.screen.name
        AppConfig.saveConfig()
        appendMessage(`SaveWindowGeometry: ${JSON.stringify(AppConfig.WindowConfig)}`, Enum.MessageType.IncomingRadioPrimary, getChatTabIndex("Logs"))
    }

    Component.onCompleted: {
        focusOrCreateTab("Logs")
        restoreSettings()
    }
}
