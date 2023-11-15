import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Shapes
import QtQml.Models

Item {
    id: control

    property string fieldLabel: ""
    property bool initializing: true

    property alias model: comboBox.model
    property alias textRole: comboBox.textRole
    property alias valueRole: comboBox.valueRole
    property alias currentIndex: comboBox.currentIndex

    signal selectedValueChanged(string value)

    Layout.preferredHeight: 50
    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
    Layout.fillWidth: true

    Component.onCompleted: initializing = false

    function indexOfValue(value) {
        return comboBox.indexOfValue(value)
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
                comboBox.forceActiveFocus()
            }
        }
    }

    ComboBox {
        id: comboBox

        y: labelControl.visible ? 20 : 0
        anchors.left: parent.left
        anchors.right: parent.right
        textRole: control.textRole
        valueRole: control.valueRole
        model: control.model

        onCurrentIndexChanged: {
            if(!initializing) {
                control.selectedValueChanged(comboBox.textAt(currentIndex))
            }
        }

        background: Rectangle {
            border.color: "#babfc4"
            color: "#ffffff"
        }

        Rectangle {
            anchors.bottom: comboBox.bottom
            width: parent.width
            height: 2
            color: "#0164AD"
            visible: comboBox.activeFocus
        }

        indicator: ColorImage {
            source: "qrc:/Resources/Icons/ChevronDown.svg"
            smooth: false
            height: 24
            width: 24
            x: comboBox.width - width - comboBox.padding - 3
            y: comboBox.topPadding + (comboBox.availableHeight - height) / 2
            color: {
                if(comboBox.visualFocus || comboBox.activeFocus || comboBox.popup.opened) {
                    return "#141618"
                }
                return "#5C5C5C"
            }
        }

        delegate: ItemDelegate {
            id: delegate
            width: comboBox.width
            padding: 5
            height: 25
            hoverEnabled: true
            highlighted: comboBox.highlightedIndex === index
            contentItem: Text {
                text: modelData != null ? (comboBox.textRole ? modelData[comboBox.textRole] : modelData) : null
                font.pixelSize: 13
                color: hovered ? "#ffffff" : "#000000"
                verticalAlignment: Text.AlignVCenter
                renderType: Text.NativeRendering
            }
            background: Rectangle {
                color: delegate.hovered ? "#0164AD" : "transparent"
            }
        }

        contentItem: Text {
            padding: 5
            clip: true
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            color: "#000000"
            renderType: Text.NativeRendering
            text: comboBox.displayText
        }

        popup: Popup {
            y: comboBox.height - 1
            width: comboBox.width
            implicitHeight: contentItem.implicitHeight
            padding: 0
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: comboBox.popup.visible ? comboBox.delegateModel : null
                currentIndex: comboBox.highlightedIndex
                ScrollBar.vertical: ScrollBar{}
            }
            background: Rectangle {
                border.color: "#0164AD"
            }
        }

        Component.onCompleted: currentIndex = find(currentText)
    }
}
