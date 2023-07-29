import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import org.vatsim.xpilot

Item {
    id: root

    property int currentTabIndex: 0
    property variant model: ListModel {}

    property color frameColor: "#5C5C5C"
    property color fillColor: "#141618"

    property string selectedMessageText: ""
    property string callsign: ""

    signal commandSubmitted(command: string, tabIndex: int)

    onCurrentTabIndexChanged: tabListView.currentIndex = currentTabIndex

    ListView {
        id: tabListView
        model: root.model
        delegate: tabDelegate
        width: parent.width
        height: 30
        orientation: ListView.Horizontal
        spacing: -1
        clip: true
        z: 1
    }

    Item {
        id: contentArea
        anchors.fill: parent
        anchors.topMargin: 29

        Repeater {
            model: root.model
            delegate: messagesDelegate
            anchors.fill: parent
            anchors.margins: 10
        }
    }

    Component {
        id: messagesDelegate

        Rectangle {
            id: messageComponent
            color: "transparent"
            border.color: frameColor
            anchors.fill: parent
            visible: index === tabListView.currentIndex

            GridLayout {
                anchors.fill: parent
                rows: 2
                columns: 1

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: 10
                    Layout.leftMargin: 5
                    Layout.rightMargin: 10
                    Layout.bottomMargin: 5
                    Layout.column: 0
                    Layout.row: 0

                    Item {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        width: parent.width
                        height: parent.height
                        clip: true

                        ListView {
                            id: view
                            anchors.fill: parent
                            model: messages

                            delegate: TextArea {
                                id: textDelegate
                                text: message
                                wrapMode: TextArea.Wrap
                                width: view.width
                                padding: 0
                                font.family: robotoMono.name
                                font.pixelSize: 13
                                renderType: TextArea.NativeRendering
                                textFormat: TextArea.RichText

                                Connections {
                                    target: selectionArea
                                    function onSelectionChanged() {
                                        updateSelection()
                                    }
                                }

                                function updateSelection() {
                                    if (selectionArea.realStartIndex === -1 || selectionArea.realEndIndex === -1)
                                        return;
                                    if (index < selectionArea.realStartIndex || index > selectionArea.realEndIndex)
                                        textDelegate.select(0, 0);
                                    else if (index > selectionArea.realStartIndex && index < selectionArea.realEndIndex)
                                        textDelegate.selectAll();
                                    else if (index === selectionArea.realStartIndex && index === selectionArea.realEndIndex)
                                        textDelegate.select(selectionArea.realStartPos, selectionArea.realEndPos);
                                    else if (index === selectionArea.realStartIndex)
                                        textDelegate.select(selectionArea.realStartPos, textDelegate.length);
                                    else if (index === selectionArea.realEndIndex)
                                        textDelegate.select(0, selectionArea.realEndPos);
                                }
                            }

                            ScrollBar.vertical: ScrollBar {
                                id: scrollBar
                                policy: ScrollBar.AsNeeded
                            }

                            onCountChanged: {
                                positionViewAtEnd()
                                currentIndex = count - 1
                            }

                            function indexAtRelative(x, y) {
                                return indexAt(x + contentX, y + contentY)
                            }
                        }

                        MouseArea {
                            id: selectionArea

                            property int selStartIndex: 0
                            property int selEndIndex: 0
                            property int selStartPos: 0
                            property int selEndPos: 0
                            property bool mousePressed: false
                            property bool selectionMade: false

                            property int realStartIndex: Math.min(selectionArea.selStartIndex, selectionArea.selEndIndex)
                            property int realEndIndex: Math.max(selectionArea.selStartIndex, selectionArea.selEndIndex)
                            property int realStartPos:
                            (selectionArea.selStartIndex < selectionArea.selEndIndex)
                                ? selectionArea.selStartPos : selectionArea.selEndPos
                            property int realEndPos:
                            (selectionArea.selStartIndex < selectionArea.selEndIndex)
                                ? selectionArea.selEndPos : selectionArea.selStartPos

                            signal selectionChanged

                            anchors.fill: parent
                            enabled: !scrollBar.hovered
                            cursorShape: enabled ? Qt.IBeamCursor : Qt.ArrowCursor
                            hoverEnabled: true

                            function selectAllText() {
                                selectedMessageText = "";
                                for (var i = 0; i < view.count; i++) {
                                    var item = view.itemAtIndex(i);
                                    if (item) {
                                        item.selectAll();
                                        if (item.selectedText !== "") {
                                            if (selectedMessageText !== "") {
                                                selectedMessageText += "\n"
                                            }
                                            selectedMessageText += item.selectedText;
                                        }
                                    }
                                }
                            }

                            function indexAndPos(x, y) {
                                const index = view.indexAtRelative(x, y);
                                if (index === -1)
                                    return;
                                const item = view.itemAtIndex(index);
                                const relItemY = item.y - view.contentY;
                                const pos = item.positionAt(x, y - relItemY);
                                return [index, pos];
                            }

                            onPressed: {
                                var result = indexAndPos(mouseX, mouseY);
                                if (result !== undefined) {
                                    selStartIndex = result[0];
                                    selStartPos = result[1];
                                    mousePressed = true;
                                    selectionMade = false;
                                    selectionChanged();
                                }
                            }

                            onPositionChanged: {
                                var delegate = view.indexAtRelative(mouseX, mouseY)
                                if (delegate !== undefined) {
                                    var item = view.itemAtIndex(delegate)
                                    if (item) {
                                        var relItemY = item.y - view.contentY
                                        var link = item.linkAt(mouseX, mouseY - relItemY)
                                        selectionArea.cursorShape = link ? Qt.PointingHandCursor : Qt.IBeamCursor
                                    }
                                }
                                if (mousePressed) {
                                    var result = indexAndPos(mouseX, mouseY);
                                    if (result !== undefined) {
                                        selEndIndex = result[0];
                                        selEndPos = result[1];
                                        if (!selectionMade) {
                                            selectionMade = true;
                                        }
                                        selectionChanged();
                                    }
                                }
                            }

                            onReleased: {
                                mousePressed = false;
                            }

                            onClicked: {
                                var delegate = view.indexAtRelative(mouseX, mouseY)
                                if (delegate !== undefined) {
                                    var item = view.itemAtIndex(delegate)
                                    if (item) {
                                        var relItemY = item.y - view.contentY
                                        var link = item.linkAt(mouseX, mouseY - relItemY)
                                        if (link !== "") {
                                            // clear text selections
                                            for (var i = realStartIndex; i <= realEndIndex; i++) {
                                                var selectedItem = view.itemAtIndex(i);
                                                if(selectedItem !== null) {
                                                    selectedItem.select(0, 0);
                                                }
                                            }
                                            Qt.openUrlExternally(link)
                                            return
                                        }
                                    }
                                }
                                if (!selectionMade) {
                                    selStartIndex = selEndIndex = selStartPos = selEndPos = 0;
                                    selectionChanged();
                                }
                            }

                            onDoubleClicked: {
                                selectAllText()
                            }

                            onSelectionChanged: {
                                selectedMessageText = ""
                                for (var i = realStartIndex; i <= realEndIndex; i++) {
                                    var item = view.itemAtIndex(i);
                                    if (item && item.selectedText !== "") {
                                        if (selectedMessageText !== "")
                                            selectedMessageText += "\n";
                                        selectedMessageText += item.selectedText;
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.column: 0
                    Layout.row: 1
                    Layout.maximumHeight: 30
                    Layout.minimumHeight: 30
                    clip: true

                    TextField {
                        id: commandLine
                        Layout.bottomMargin: -5
                        Layout.rightMargin: -5
                        Layout.leftMargin: -5
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        topPadding: 0
                        padding: 6
                        selectByMouse: true
                        font.family: robotoMono.name
                        font.pixelSize: 13
                        renderType: Text.NativeRendering
                        color: "#ffffff"
                        selectionColor: "#0164ad"
                        selectedTextColor: "#ffffff"
                        focus: true

                        onVisibleChanged: {
                            if(messageComponent.visible) {
                                commandLine.forceActiveFocus()
                            }
                        }

                        background: Rectangle {
                            color: "transparent"
                            border.color: frameColor
                        }

                        property var commandHistory: []
                        property int historyIndex: -1
                        property string commandLineValue: ""

                        Keys.onPressed: function(event) {
                            if (event.key === Qt.Key_Escape) {
                                commandLine.clear()
                            }
                            else if (event.key === Qt.Key_C && event.modifiers & Qt.ControlModifier) {
                                clipboard.setText(selectedMessageText)
                            }
                            else if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                                if (commandLine.text.length === 0)
                                    return

                                commandSubmitted(commandLine.text, root.currentTabIndex)

                                commandHistory.unshift(commandLine.text)
                                commandLine.clear()
                                historyIndex = -1
                            }
                            else if (event.key === Qt.Key_Down) {
                                if (historyIndex === -1) {
                                    commandLineValue = commandLine.text
                                }

                                historyIndex--

                                if (historyIndex < 0) {
                                    historyIndex = -1
                                    commandLine.text = commandLineValue
                                    return
                                }

                                commandLine.text = commandHistory[historyIndex]
                                return
                            }
                            else if (event.key === Qt.Key_Up) {
                                if (historyIndex === -1) {
                                    commandLineValue = commandLine.text
                                }

                                historyIndex++

                                if (historyIndex >= commandHistory.length) {
                                    historyIndex = -1
                                    commandLine.text = commandLineValue
                                    return
                                }

                                commandLine.text = commandHistory[historyIndex]
                                return
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: tabDelegate

        Item {
            id: tab

            property var view: ListView.view
            property int tabIndex: index

            implicitWidth: isCloseable ? (tabLabel.width + 40) : (tabLabel.width + 20)
            implicitHeight: 30

            Rectangle {
                id: topRect
                anchors.fill: parent
                radius: 5
                color: fillColor
                border.width: 1
                border.color: frameColor
                antialiasing: false
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

            // remove weird line that runs through the middle of the tab
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
                visible: tabIndex === view.currentIndex
                width: tab.width - 2
                height: 2
                color: fillColor
                y: parent.height - 2
                x: (tab.width - tab.width) + 1
            }

            Text {
                id: tabLabel
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                text: title
                renderType: Text.NativeRendering
                font.family: robotoMono.name
                font.pixelSize: 13
                color: tabIndex === view.currentIndex ? "white" : (hasMessageWaiting ? "yellow" : frameColor)
            }

            TransparentButton {
                id: btnClose
                visible: isCloseable
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 0
                z: 100

                icon.source: "../Icons/CloseIcon.svg"
                icon.color: "transparent"
                icon.width: 18
                icon.height: 18
                onHoveredChanged: hovered ? icon.color = "white" : icon.color = "transparent"

                MouseArea {
                    anchors.fill: btnClose
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.model.remove(tabIndex)
                        view.currentIndex = root.currentTabIndex = 0
                    }
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    view.currentIndex = root.currentTabIndex = tabIndex
                    hasMessageWaiting = false
                    root.callsign = title
                }
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
