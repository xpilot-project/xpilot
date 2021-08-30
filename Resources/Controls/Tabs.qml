import QtQuick 2.12

ListView {
    id: root
    model: []
    currentIndex: 0
    orientation: ListView.Horizontal
    interactive: false
    clip: true
    height: 33
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.rightMargin: 15
    anchors.leftMargin: 15
    anchors.topMargin: 15
    spacing: -1

    delegate: Item {
        width: t_metrics.advanceWidth + 25
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
                text: modelData
                font.pixelSize: 13
                font.family: robotoMono.name
                renderType: Text.NativeRendering
                anchors.centerIn: parent
                color: currentIndex == index? '#ffffff': '#42494e'
            }

            TextMetrics {
                id: t_metrics
                font: label.font
                text: label.text
            }

            Rectangle {
                visible: currentIndex == index;
                anchors {
                    bottom: parent.bottom
                    right: parent.left
                }
                width: root.width
                height: 0.02 * root.height
                color: '#5C5C5C'
            }

            Rectangle {
                visible: currentIndex == index;
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
                onClicked:  currentIndex = index
                cursorShape: Qt.PointingHandCursor
                preventStealing: true
            }
        }
    }
}
