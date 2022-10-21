import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Rectangle {
    id: root

    property real minValue: 0
    property real maxValue: 100
    property real volume: 50
    property bool showPercent: true
    property string comLabel: "COM1"
    signal volumeValueChanged(real volume)

    width: 295
    height: 30

    Row {
        Label {
            id: label
            text: comLabel
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 13
            renderType: Text.NativeRendering
        }

        Slider {

            id: control
            value: volume
            from: minValue
            to: maxValue

            onMoved: volumeValueChanged(value)

            background: Rectangle {
                x: control.leftPadding
                y: control.topPadding + control.availableHeight / 2 - height / 2
                implicitWidth: 230
                implicitHeight: 4
                width: control.availableWidth
                height: implicitHeight
                radius: 2
                color: "#bdbebf"

                Rectangle {
                    width: control.visualPosition * parent.width
                    height: parent.height
                    color: "#0164AD"
                    radius: 2
                }
            }

            handle: Rectangle {
                x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
                y: control.topPadding + control.availableHeight / 2 - height / 2
                implicitWidth: 18
                implicitHeight: 18
                radius: 13
                color: control.pressed ? "#f0f0f0" : "#f6f6f6"
                border.color: "#bdbebf"
            }
        }

        Label {
            id: label1
            text: Math.round(control.value) + (showPercent ? "%" : "")
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 13
            renderType: Text.NativeRendering
        }
    }
}
