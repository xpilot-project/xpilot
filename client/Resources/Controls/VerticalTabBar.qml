import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

TabBar {
    id: control
    width: parent.width
    background: Rectangle {
        color: "#f0f0f0"
    }

    contentItem: ListView {
        width: control.width
        height: control.height
        model: control.contentModel
        currentIndex: control.currentIndex

        spacing: control.spacing
        orientation: ListView.Vertical
        boundsBehavior: Flickable.StopAtBounds
    }
}
