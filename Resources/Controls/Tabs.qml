import QtQuick 2.12

ListView {
    id: root
    currentIndex: 0
    orientation: ListView.Horizontal
    interactive: false
    clip: true
    height: 30
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.rightMargin: 15
    anchors.leftMargin: 15
    anchors.topMargin: 15
    spacing: -1

    delegate: Item {
        width: disposable ? metrics.advanceWidth + 45 :  metrics.advanceWidth + 25
        height: root.height
        y: root.height - height

        Item {
            anchors.fill: parent

            Rectangle {
                width: parent.width
                height: 2 * parent.height
                border.width: 1
                border.color: '#5C5C5C'
                radius: 8
                color: 'transparent'
            }

            Text {
                id: label
                text: name
                font.pixelSize: 13
                font.family: robotoMono.name
                renderType: Text.NativeRendering
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                rightPadding: disposable ? 15 : 0
                height: 20
                color: currentIndex == index ? '#ffffff' : '#42494e'
            }

            WindowControlButton {
                id: btnClose
                visible: disposable
                anchors.right: parent.right
                icon.source: "../Icons/CloseIcon.svg"
                icon.color: "transparent"
                icon.width: 16
                icon.height: 16
                onHoveredChanged: hovered ? icon.color = "white" : icon.color = "transparent"

                MouseArea {
                    id: btnCloseMouseArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: {

                    }
                }
            }

            TextMetrics {
                id: metrics
                font: label.font
                text: label.text
            }

            Rectangle {
                visible: currentIndex == index
                anchors {
                    bottom: parent.bottom
                    right: parent.left
                }
                width: root.width
                height: 0.02 * root.height
                color: '#5C5C5C'
            }

            Rectangle {
                visible: currentIndex == index
                anchors {
                    bottom: parent.bottom
                    left: parent.right
                }
                width: root.width
                height: 0.02 * root.height
                color: '#5C5C5C'
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                enabled: currentIndex != index
                onClicked: currentIndex = index
                cursorShape: Qt.PointingHandCursor
                preventStealing: true
            }
        }
    }
}
