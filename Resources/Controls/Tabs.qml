import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

TabView {
    id: tabView
    anchors.fill: parent
    anchors.margins: 10

    style: TabViewStyle {
        property color frameColor: "#5C5C5C"
        property color fillColor: "#141618"
        frameOverlap: 1

        frame: Rectangle {
            color: "transparent"
            border.color: "#5c5c5c"
            border.width: 1
            anchors.fill: parent
        }

        tab: Rectangle {
            id: tabRect
            implicitWidth: text.width + 50
            implicitHeight: 30
            color: "transparent"

            Rectangle {
                id: topRect
                anchors.fill: parent
                radius: 7
                color: fillColor
                border.width: 1
                border.color: frameColor
            }

            Rectangle {
                id: bottomRect
                anchors.bottom: parent.bottom
                anchors.left: topRect.left
                anchors.right: topRect.right
                height: 1 / 2 * parent.height
                color: fillColor
                border.width: 1
                border.color: frameColor
            }

            // cleans up tab, otherwise there's a weird line that runs
            // through the middle of the tab
            Rectangle {
                anchors {
                    fill: bottomRect
                    leftMargin: bottomRect.border.width
                    bottomMargin: bottomRect.border.width
                    rightMargin: bottomRect.border.width
                }
                color: fillColor
            }

            // hides bottom border on active tab
            Rectangle {
                visible: styleData.selected
                width: tabRect.width - 2
                height: 2
                color: fillColor
                y: parent.height - 2
                x: (tabRect.width - tabRect.width) + 1
            }

            Text {
                id: text
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                text: styleData.title
                color: styleData.selected ? "white" : frameColor
                font.family: robotoMono.name
                font.pixelSize: 13
                renderType: Text.NativeRendering
            }

            WindowControlButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 0

                icon.source: "../Icons/CloseIcon.svg"
                icon.color: "transparent"
                icon.width: 18
                icon.height: 18
                onHoveredChanged: hovered ? icon.color = "white" : icon.color = "transparent"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: tabView.removeTab(styleData.index)
                }
            }
        }
    }
}
