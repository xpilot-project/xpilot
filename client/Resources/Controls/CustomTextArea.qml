import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextArea {
    id: textField
    background: Rectangle {
        border.color: textField.focus ? '#565E64' : '#babfc4'
        color: textField.enabled ? 'transparent' : '#E6E7E8'
    }
    color: '#333333'
    placeholderTextColor: "#babfc4"
    selectionColor: "#0164AD"
    selectedTextColor: "#ffffff"
    selectByMouse: true
    renderType: Text.NativeRendering
}
