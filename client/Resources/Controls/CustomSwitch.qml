import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Switch {
    id: control

    property string tooltipText: ""

    indicator: Rectangle {
        implicitWidth: 30
        implicitHeight: 15
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 18
        color: control.checked ? "#0164AD" : "#ffffff"
        border.color: control.checked ? "#0164AD" : "#ADB5BD"
        z: 100

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
        id: label
        text: control.text
        font.pixelSize: 13
        opacity: enabled ? 1.0 : 0.3
        color: "#000000"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
        wrapMode: Text.WordWrap
        renderType: Text.NativeRendering

        MouseArea {
            anchors.fill: parent
            hoverEnabled: tooltipText !== ""
            propagateComposedEvents: true

            onEntered: {
                label.ToolTip.visible = true
            }

            onExited: {
                label.ToolTip.visible = false
            }

            onClicked: (mouse) => mouse.accepted = false
            onPressed: (mouse) => mouse.accepted = false
            onReleased: (mouse) => mouse.accepted = false
            onDoubleClicked: (mouse) => mouse.accepted = false
            onPositionChanged: (mouse) => mouse.accepted = false
            onPressAndHold: (mouse) => mouse.accepted = false
        }

        ToolTip.visible: false
        ToolTip.text: tooltipText
    }
}
