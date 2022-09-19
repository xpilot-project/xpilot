import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextField {
    id: textField
    anchors.left: parent.left
    anchors.right: parent.right
    background: Rectangle {
        border.color: textField.focus ? '#565E64' : '#babfc4'
        color: textField.enabled ? 'transparent' : '#E6E7E8'
    }
    font.pixelSize: 13
    color: '#333333'
    height: 30
    placeholderTextColor: "#babfc4"
    selectionColor: "#0164AD"
    selectedTextColor: "#ffffff"
    selectByMouse: true
    renderType: Text.NativeRendering
}
