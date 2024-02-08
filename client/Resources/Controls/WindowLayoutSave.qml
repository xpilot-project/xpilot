import QtQuick
import QtQuick.Window

import org.vatsim.xpilot

Item {
    id: root

    property QtObject target: Window.window
    property bool initialized: false

    Connections {
        target: root.target
        function onXChanged() { saveSettings() }
        function onYChanged() { saveSettings() }
        function onWidthChanged() { saveSettings() }
        function onHeightChanged() { saveSettings() }
        function onVisibilityChanged() { saveSettings() }
    }

    function centerWindow() {
        AppConfig.WindowConfig.X = Screen.width / 2 - target.width / 2
        AppConfig.WindowConfig.Y = Screen.height / 2 - target.height / 2
        AppConfig.WindowConfig.Maximized = false
    }

    function restoreSettings() {
        let screenFound = false
        for(let i = 0; i < Qt.application.screens.length; i++) {
            const screen = Qt.application.screens[i]
            if(screen.name === AppConfig.WindowConfig.ScreenName) {
                const isInWidth = AppConfig.WindowConfig.X >= screen.virtualX && AppConfig.WindowConfig.X <= screen.virtualX + screen.width
                const isInHeight = AppConfig.WindowConfig.Y >= screen.virtualY && AppConfig.WindowConfig.Y <= screen.virtualY + screen.width
                if(isInWidth && isInHeight) {
                    target.screen = screen
                    screenFound = true
                    break
                }
            }
        }

        if(!screenFound) {
            centerWindow()
        }

        // Ensure window position is valid
        // if (AppConfig.WindowConfig.X < 0 || AppConfig.WindowConfig.X >= Screen.desktopAvailableWidth - target.width) {
        //     centerWindow()
        // }
        // if (AppConfig.WindowConfig.Y < 0 || AppConfig.WindowConfig.Y >= Screen.desktopAvailableHeight - target.height) {
        //     centerWindow()
        // }

        if (AppConfig.WindowConfig.Width > Screen.desktopAvailableWidth) {
            AppConfig.WindowConfig.Width = Screen.desktopAvailableWidth - AppConfig.WindowConfig.X
        }
        if (AppConfig.WindowConfig.Height > Screen.desktopAvailableHeight) {
            AppConfig.WindowConfig.Height = Screen.desktopAvailableHeight - AppConfig.WindowConfig.Y
        }

        // Apply window geometry
        if(AppConfig.WindowConfig.Width && AppConfig.WindowConfig.Height && !initialized) {
            target.x = AppConfig.WindowConfig.X
            target.y = AppConfig.WindowConfig.Y
            target.width = AppConfig.WindowConfig.Width
            target.height = AppConfig.WindowConfig.Height
            target.visibility = AppConfig.WindowConfig.Maximized ? Window.FullScreen : Window.Windowed
            initialized = true
        }
    }

    function saveSettings() {
        if(!initialized || !target.visible) return
        AppConfig.WindowConfig.X = target.x
        AppConfig.WindowConfig.Y = target.y
        AppConfig.WindowConfig.Width = target.width
        AppConfig.WindowConfig.Height = target.height
        AppConfig.WindowConfig.Maximized = (target.visibility === Window.FullScreen || target.visibility === Window.Maximized)
        AppConfig.WindowConfig.ScreenName = target.screen.name
        AppConfig.saveConfig()
    }

    Component.onCompleted: restoreSettings()
}
