import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Button {
    id: button
    anchors.verticalCenter: parent.verticalCenter
    checked: false
    checkable: false
    background: Rectangle {
        color: "transparent"
    }
}
