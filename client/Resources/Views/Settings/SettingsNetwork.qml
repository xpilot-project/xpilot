import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../../Controls"
import org.vatsim.xpilot

Item {

    signal applyChanges()

    Component.onCompleted: {
        vatsimId.fieldValue = AppConfig.VatsimId
        vatsimPassword.fieldValue = AppConfig.VatsimPasswordDecrypted
        realName.fieldValue = AppConfig.Name
        homeAirport.fieldValue = AppConfig.HomeAirport
        networkServerList.model = AppConfig.CachedServers
    }

    ColumnLayout {
        spacing: 10
        width: 240

        CustomTextField {
            id: vatsimId
            fieldLabel: "VATSIM ID:"
            onValueChanged: function(value) {
                AppConfig.VatsimId = value
                applyChanges()
            }
            validator: RegularExpressionValidator {
                regularExpression: /[0-9]+/
            }
        }

        CustomTextField {
            id: vatsimPassword
            fieldLabel: "VATSIM Password:"
            isPasswordField: true
            onValueChanged: function(value) {
                AppConfig.VatsimPasswordDecrypted = value
                applyChanges()
            }
        }

        CustomTextField {
            id: realName
            fieldLabel: "Your Name:"
            onValueChanged: function(value) {
                AppConfig.Name = value
                applyChanges()
            }
        }

        CustomTextField {
            id: homeAirport
            fieldLabel: "Your Home Airport:"
            maximumLength: 4
            isUppercase: true
            onValueChanged: function(value) {
                AppConfig.HomeAirport = value
                applyChanges()
            }
            validator: RegularExpressionValidator {
                regularExpression: /[a-zA-Z0-9]{4}/
            }
        }

        CustomComboBox {
            id: networkServerList
            fieldLabel: "VATSIM Server:"
            textRole: "name"
            valueRole: "address"
            onSelectedValueChanged: function(value) {
                AppConfig.ServerName = value
                applyChanges()
            }
        }
    }
}
