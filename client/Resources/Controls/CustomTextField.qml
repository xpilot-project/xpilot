import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: control

    property string fieldLabel: ""
    property bool isPasswordField: false
    property bool isUppercase: false
    property string fieldValue: ""
    property int maximumLength: 32767
    property var validator: null

    signal valueChanged(string value)

    property bool initializing: true

    Layout.preferredHeight: 50
    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
    Layout.fillWidth: true

    Component.onCompleted: initializing = false

    function trimLineBreaks(value) {
        return value.replace(/[\n\r]/g, "")
    }

    Label {
        id: labelControl
        color: "#000000"
        text: fieldLabel
        font.pixelSize: 13
        renderType: Text.NativeRendering
        visible: fieldLabel !== ""
        MouseArea {
            anchors.fill: labelControl
            onClicked: {
                textField.forceActiveFocus()
            }
        }
    }

    TextField {
        id: textField

        y: labelControl.visible ? 20 : 0
        height: 28
        anchors.left: parent.left
        anchors.right: parent.right
        echoMode: isPasswordField ? TextInput.Password : TextInput.Normal
        font.pixelSize: 13
        color: "#000000"
        selectByMouse: true
        selectionColor: "#0164AD"
        selectedTextColor: "#ffffff"
        renderType: TextField.NativeRendering
        text: fieldValue
        maximumLength: control.maximumLength
        leftPadding: 6

        background: Rectangle {
            border.color: "#babfc4"
            color: "#ffffff"
        }

        Rectangle {
            anchors.bottom: textField.bottom
            width: parent.width
            height: 2
            color: "#0164AD"
            visible: textField.activeFocus
        }

        onTextChanged: {
            if(isUppercase) {
                textField.text = textField.text.toUpperCase()
            }

            if(!initializing) {
                valueChanged(textField.text)
            }

            textField.text = trimLineBreaks(textField.text)
            fieldValue = textField.text
        }

        validator: control.validator
    }
}
