import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Rectangle {
    id: control
    property variant internalModel
    color: 'transparent'
    border.color: '#5C5C5C'
    GridLayout {
        anchors.fill: parent
        rows: 2
        columns: 1

        RowLayout {
            id: messages
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 0
            Layout.preferredHeight: 300
            Layout.fillHeight: true

            ScrollView
            {
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                leftPadding: 10
                topPadding: 5
                rightPadding: 10
                bottomPadding: 5

                ListView {
                    id: listView
                    model: control.internalModel
                    delegate: Rectangle {
                        anchors.left: listView.contentItem.left
                        anchors.right: listView.contentItem.right
                        height: text.contentHeight
                        color: 'transparent'
                        Text {
                            id: text
                            text: modelData
                            width: parent.width
                            wrapMode: Text.WordWrap
                            renderType: Text.NativeRendering
                            font.family: robotoMono.name
                            font.pixelSize: 13
                            color: '#ffffff'
                        }
                    }
                    onCountChanged: {
                        var newIndex = count - 1
                        positionViewAtEnd()
                        currentIndex = newIndex
                    }
                }
            }
        }

        RowLayout {
            id: commandLine
            Layout.fillWidth: true
            clip: true
            Layout.column: 0
            Layout.row: 1
            Layout.maximumHeight: 30
            Layout.minimumHeight: 30

            TextField {
                id: textField
                font.pixelSize: 13
                font.family: robotoMono.name
                renderType: Text.NativeRendering
                color: '#ffffff'
                selectionColor: "#0164AD"
                selectedTextColor: "#ffffff"
                topPadding: 0
                padding: 6
                Layout.bottomMargin: -5
                Layout.rightMargin: -5
                Layout.leftMargin: -5
                Layout.fillHeight: true
                Layout.fillWidth: true
                background: Rectangle {
                    color: 'transparent'
                    border.color: '#5C5C5C'
                }
                Keys.onPressed: {
                    if(event.key === Qt.Key_Escape) {
                        text = ""
                    }
                    if(event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                        text = ""
                    }
                }
            }
        }
    }
}
