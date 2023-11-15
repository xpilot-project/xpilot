import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.vatsim.xpilot
import "../../Controls"

Item {

    signal applyChanges()

    Component.onCompleted: {
        switchAutoModeC.checked = AppConfig.AutoModeC
        switchKeepWindowVisible.checked = AppConfig.KeepWindowVisible
    }

    ColumnLayout {
        spacing: 10
        width: parent.width

        CustomSwitch {
            id: switchAutoModeC
            text: "Automatically set transponder to Mode C on takeoff"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.AutoModeC = switchAutoModeC.checked
                applyChanges()
            }
        }

        CustomSwitch {
            id: switchKeepWindowVisible
            text: "Keep xPilot window visible"
            font.pixelSize: 13
            onCheckedChanged: {
                AppConfig.KeepWindowVisible = switchKeepWindowVisible.checked
                applyChanges()
            }
        }
    }
}
