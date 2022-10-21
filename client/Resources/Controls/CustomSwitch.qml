import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Switch {
    id: control
    text: qsTr("Switch")

    indicator: Rectangle {
        implicitWidth: 30
        implicitHeight: 15
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 18
        color: control.checked ? "#0164AD" : "#ffffff"
        border.color: control.checked ? "#0164AD" : "#ADB5BD"

        Rectangle {
            x: control.checked ? parent.width - width : 0
            width: 15
            height: 15
            radius: 18
            color: control.checked ? "#ffffff" : "#ADB5BD"
            border.color: control.checked ? "#0164AD" : "#ADB5BD"
        }
    }

    contentItem: Text {
        text: control.text
        font.pixelSize: 13
        opacity: enabled ? 1.0 : 0.3
        color: "#333333"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
        wrapMode: Text.WordWrap
        renderType: Text.NativeRendering
    }
}
