import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

CheckBox {
    id: control
    indicator.height: 15
    indicator.width:  15
    padding: 0
    contentItem: Text {
        text: control.text
        font.pixelSize: 13
        opacity: enabled ? 1.0 : 0.3
        color: "#0164AD"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
        renderType: Text.NativeRendering
    }
}
