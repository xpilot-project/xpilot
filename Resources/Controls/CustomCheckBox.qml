import QtQuick 2.12
import QtQuick.Controls 2.12

CheckBox {
    id: control
    indicator.height: 15
    indicator.width:  15
    padding: 0
    contentItem: Text {
        text: control.text
        font.family: control.font.family
        font.pixelSize: 13
        opacity: enabled ? 1.0 : 0.3
        color: "#0164AD"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
