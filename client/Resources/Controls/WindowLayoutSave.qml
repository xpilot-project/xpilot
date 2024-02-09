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
        }
        if (AppConfig.WindowConfig.Height > Screen.desktopAvailableHeight) {
            AppConfig.WindowConfig.Height = Screen.desktopAvailableHeight - AppConfig.WindowConfig.Y
        }

        // Apply window geometry
        if(AppConfig.WindowConfig.Width && AppConfig.WindowConfig.Height && !initialized) {
            window.x = AppConfig.WindowConfig.X
            window.y = AppConfig.WindowConfig.Y
            window.width = AppConfig.WindowConfig.Width
            window.height = AppConfig.WindowConfig.Height
            window.visibility = AppConfig.WindowConfig.Maximized ? Window.FullScreen : Window.Windowed
            initialized = true
        }

        let screenFound = false
        for(let i = 0; i < Qt.application.screens.length; i++) {
            const screen = Qt.application.screens[i]
            if(screen.name === AppConfig.WindowConfig.ScreenName) {
                window.screen = screen
                screenFound = true
                break
            }
        }

        if(!screenFound) {
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
    }

    Component.onCompleted: restoreSettings()
}
