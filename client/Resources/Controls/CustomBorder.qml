import QtQuick

Rectangle {
    id: control

    property bool commonBorder: true

    property int leftBorderWidth: 1
    property int rightBorderWidth: 1
    property int topBorderWidth: 1
    property int bottomBorderWidth: 1

    property int commonBorderWidth: 1
    property string borderColor: "black"

    z: -1
    color: borderColor

    anchors {
        left: parent.left
        right: parent.right
        top: parent.top
        bottom: parent.bottom

        topMargin: commonBorder ? -commonBorderWidth : -topBorderWidth
        bottomMargin: commonBorder ? -commonBorderWidth : -bottomBorderWidth
        leftMargin: commonBorder ? -commonBorderWidth : -leftBorderWidth
        rightMargin: commonBorder ? -commonBorderWidth : -rightBorderWidth
    }
}
