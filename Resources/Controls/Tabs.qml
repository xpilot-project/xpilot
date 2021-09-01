import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Item {
    width: 400
    height: 300
    TabView {
        id: tabView
        anchors.fill: parent
        anchors.margins: 4
        Tab {
            title: "first"
        }
        Tab {
            title: "second"
        }
        Tab {
            title: "third"
        }
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
                color: "transparent"
                implicitWidth: Math.max(text.width + 24, 80)
                implicitHeight: 30
                Rectangle {
                    height: 1
                    width: parent.width
                    color: frameColor
                    radius: 8
                }
                Rectangle {
                    height: parent.height
                    width: 1
                    color: frameColor
                    radius: 8
                }
                Rectangle {
                    x: parent.width - 1
                    height: parent.height
                    width: 1
                    color: frameColor
                }
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
                    anchors.leftMargin: 6
                    text: styleData.title
                    color: styleData.selected ? "black" : "white"
                }
//                Rectangle {
//                    anchors.right: parent.right
//                    anchors.verticalCenter: parent.verticalCenter
//                    anchors.rightMargin: 4
//                    implicitWidth: 16
//                    implicitHeight: 16
//                    radius: width / 2
//                    color: control.hovered ? "#eee" : "#ccc"
//                    border.color: "#5C5C5C"
//                    Text {
//                        text: "X"
//                        anchors.centerIn: parent
//                        color: "gray"
//                    }
//                    MouseArea {
//                        anchors.fill: parent
//                        onClicked: tabView.removeTab(styleData.index)
//                    }
//                }
            }
        }
    }
} //TabView {//    id: frame
//    anchors.fill: parent
//    anchors.margins: 4
//    Tab { title: "Tab 1" }
//    Tab { title: "Tab 2" }
//    Tab { title: "Tab 3" }

//    style: TabViewStyle {
//        frameOverlap: 1
//        tab: Rectangle {
//            id: tab
//            border.color:  "#5c5c5c"
//            border.width: 1
//            color: "transparent"
//            implicitWidth: Math.max(text.width + 4, 80)
//            implicitHeight: 30
//            radius: 8
//            Text {
//                id: text
//                anchors.centerIn: parent
//                text: styleData.title
//                color: styleData.selected ? "white" : "black"
//            }
//            Rectangle {
//                visible: !styleData.selected
//                anchors {
//                    bottom: tab.bottom
//                    right: tab.left
//                }
//                width: tab.implicitWidth
//                height: 3
//                color: '#5c5c5c'
//                z: 300
//            }
//        }
//        frame: Rectangle {
//            color: "transparent"
//            border.color: "#5c5c5c"
//            border.width: 1
//            anchors.topMargin: -border.width
//            anchors.fill: parent
//        }
//    }
//}

//import QtQuick 2.12

//ListView {
//    id: root
//    currentIndex: 0
//    orientation: ListView.Horizontal
//    interactive: false
//    anchors.left: parent.left
//    anchors.right: parent.right
//    anchors.top: parent.top
//    anchors.rightMargin: 15
//    anchors.leftMargin: 15
//    anchors.topMargin: 15

//    delegate: Item {

//        Rectangle {
//            id: tabRect
//            width: disposable ? metrics.advanceWidth + 45 : metrics.advanceWidth + 25
//            height: 30
//            border.width: 1
//            border.color: '#5C5C5C'
//            radius: 8
//            color: 'transparent'
//        }

//        Text {
//            id: label
//            text: name
//            font.pixelSize: 13
//            font.family: robotoMono.name
//            renderType: Text.NativeRendering
//            verticalAlignment: Text.AlignVCenter
//            horizontalAlignment: Text.AlignLeft
//            anchors.verticalCenter: tabRect.verticalCenter
//            anchors.horizontalCenter: tabRect.horizontalCenter
//            rightPadding: disposable ? 15 : 0
//            height: 20
//            color: currentIndex == index ? '#ffffff' : '#42494e'
//        }

//        WindowControlButton {
//            id: btnClose
//            visible: disposable
//            anchors.right: tabRect.right
//            icon.source: "../Icons/CloseIcon.svg"
//            icon.color: "transparent"
//            icon.width: 16
//            icon.height: 16
//            onHoveredChanged: hovered ? icon.color = "white" : icon.color = "transparent"

//            MouseArea {
//                id: btnCloseMouseArea
//                anchors.fill: parent
//                cursorShape: Qt.PointingHandCursor
//                hoverEnabled: true
//                onClicked: {

//                }
//            }
//        }

//        TextMetrics {
//            id: metrics
//            font: label.font
//            text: label.text
//        }

//        //            Rectangle {
//        //                visible: currentIndex == index
//        //                anchors {
//        //                    bottom: parent.bottom
//        //                    right: parent.left
//        //                }
//        //                width: root.width
//        //                height: 0.02 * root.height
//        //                color: '#5C5C5C'
//        //            }

//        //            Rectangle {
//        //                visible: currentIndex == index
//        //                anchors {
//        //                    bottom: parent.bottom
//        //                    left: parent.right
//        //                }
//        //                width: root.width
//        //                height: 0.02 * root.height
//        //                color: '#5C5C5C'
//        //            }

//        MouseArea {
//            id: mouseArea
//            anchors.fill: parent
//            enabled: currentIndex != index
//            onClicked: currentIndex = index
//            cursorShape: Qt.PointingHandCursor
//            preventStealing: true
//        }
//    }
//}

