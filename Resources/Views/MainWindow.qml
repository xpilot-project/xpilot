import QtQuick 2.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.12
import QtQuick.Window 2.15
import QtQuick.Layouts 1.12
import QtWebSockets 1.2
import "../Components"
import "../Controls"

Window {
    property bool isModeC: false
    property string wsHost: "localhost"
    property string wsPort: "9000"
    property QtObject connectWindow
    property QtObject settingsWindow
    property QtObject flightPlanWindow
    property int currentTab

    id: mainWindow
    title: "xPilot"
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    width: 800
    height: 250
    minimumHeight: 250
    minimumWidth: 800
    color: "#272C2E"

    Connections {
        target: connectWindow
        function onCloseWindow() {
            connectWindow.destroy()
        }
    }

    Connections {
        target: flightPlanWindow
        function onCloseWindow() {
            flightPlanWindow.destroy()
        }
    }

    Connections {
        target: settingsWindow
        function onCloseWindow() {
            settingsWindow.destroy()
        }
    }

    Component.onCompleted: {
        //        wsClient.connect(wsHost, wsPort)
    }

    Timer {
        id: timer
        function setTimeout(cb, delayTime) {
            timer.interval = delayTime
            timer.repeat = false
            timer.triggered.connect(cb)
            timer.triggered.connect(function release() {
                timer.triggered.disconnect(cb)
                timer.triggered.disconnect(release)
            })
            timer.start()
        }
    }

    WebSocket {
        id: wsClient

        onStatusChanged: {
            switch (wsClient.status) {
            case WebSocket.Connecting:
                console.log("Connecting")
                break
            case WebSocket.Open:
                console.log("Opened")
                break
            case WebSocket.Closing:
                console.log("Closing")
                break
            case WebSocket.Closed:
                console.log("Closed")
                timer.setTimeout(function () {
                    wsClient.connect(wsHost, wsPort)
                }, 1000)
                break
            case WebSocket.Error:
                console.log("Error: " + errorString)
                break
            }
        }

        function connect(host, port) {
            console.log("Attempting connection...")
            wsClient.active = false
            wsClient.url = ""
            var address = "ws://"
            address = address.concat(host, ":", port)
            wsClient.url = address
            wsClient.active = true
        }
    }

    FontLoader {
        id: ubuntuRegular
        source: "../Fonts/Ubuntu-Regular.ttf"
    }

    FontLoader {
        id: robotoMono
        source: "../Fonts/Roboto-Mono.ttf"
    }

    Rectangle {
        id: windowFrame
        anchors.fill: parent
        color: "transparent"
        border.color: Qt.platform.os === 'osx' ? 'transparent' : '#000000'
        z: 200
    }

    GridLayout {
        anchors.fill: parent
        rowSpacing: 0
        columnSpacing: 0
        rows: 2
        columns: 2
        z: 100

        Rectangle {
            id: radioStack
            Layout.preferredWidth: 250
            Layout.preferredHeight: 75
            Layout.row: 0
            Layout.column: 0
            color: "#0164AD"

            RadioStack {
                anchors.verticalCenter: parent.verticalCenter
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 0
            }
        }

        Rectangle {
            id: toolbar
            Layout.row: 0
            Layout.column: 1
            color: "transparent"
            Layout.preferredHeight: 75
            Layout.fillWidth: true

            Toolbar {}
        }

        Rectangle {
            id: nearbyAtc
            Layout.row: 1
            Layout.column: 0
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            color: "transparent"

            NearbyAtc {
                anchors.fill: parent
            }
        }

        Rectangle {
            id: msgControl
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 1
            color: "#141618"

            Rectangle {
                anchors.fill: parent
                anchors.margins: 10
                anchors.topMargin: 39
                border.color: "#5c5c5c"
                border.width: 1
                color: "transparent"
            }

            ListModel {
                id: tabModel
                ListElement {
                    title: "Messages"
                    disposable: false
                }
                ListElement {
                    title: "Notes"
                    disposable: false
                }
            }

            Component {
                id: tabDelegate
                Item {
                    id: tab

                    property color frameColor: "#5C5C5C"
                    property color fillColor: "#141618"

                    property var view: ListView.view
                    property int itemIndex: index

                    implicitWidth: disposable ? text.width + 45 : text.width + 20
                    implicitHeight: 30

                    Rectangle {
                        id: topRect
                        anchors.fill: parent
                        radius: 8
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
                        visible: itemIndex === view.currentIndex
                        width: tab.width - 2
                        height: 2
                        color: fillColor
                        y: parent.height - 2
                        x: (tab.width - tab.width) + 1
                    }

                    Text {
                        id: text
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 10
                        text: title
                        color: itemIndex === view.currentIndex ? "white" : frameColor
                        font.family: robotoMono.name
                        font.pixelSize: 13
                    }

                    WindowControlButton {
                        id: btnClose
                        visible: disposable
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
                            onClicked: tabModel.remove(itemIndex)
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        onClicked: {
                            currentTab = itemIndex
                            view.currentIndex = itemIndex
                        }
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }

            ListView {
                model: tabModel
                delegate: tabDelegate
                anchors.fill: parent
                anchors.margins: 10
                orientation: ListView.Horizontal
                spacing: -1
                clip: true
            }

            ListModel {
                id: cliModel
                ListElement {
                    tabId: 0
                    attributes: [
                        ListElement { timestamp: "00:15:24"; message: "xPilot 2.0.0-beta.1" },
                        ListElement { timestamp: "00:15:24"; message: "Version check complete. You are running the latest version" },
                        ListElement { timestamp: "00:15:25"; message: "Waiting for X-Plane connection..." },
                        ListElement { timestamp: "00:15:27"; message: "X-Plane connection established" }
                    ]
                }
            }

            Component {
                id: cliDelegate

                Rectangle {
                    id: messages
                    color: 'transparent'
                    anchors.fill: parent
                    anchors.margins: 10
                    anchors.topMargin: 39

                    GridLayout {
                        anchors.fill: parent
                        rows: 2
                        columns: 1

                        RowLayout {
                            id: rowMessages
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 300
                            Layout.column: 0
                            Layout.row: 0

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
                                    model: attributes
                                    delegate: Rectangle {
                                        anchors.left: listView.contentItem.left
                                        anchors.right: listView.contentItem.right
                                        height: text.contentHeight
                                        color: 'transparent'
                                        visible: tabId === currentTab
                                        Text {
                                            id: text
                                            text: "[" + timestamp + "]: " + message
                                            width: parent.width
                                            wrapMode: Text.WordWrap
                                            font.family: robotoMono.name
                                            renderType: Text.NativeRendering
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
                                id: cliTextField
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
                                selectByMouse: true
                                background: Rectangle {
                                    color: 'transparent'
                                    border.color: '#5C5C5C'
                                }
                            }
                        }
                    }
                }
            }

            Repeater {
                model: cliModel
                delegate: cliDelegate
                anchors.fill: parent
                anchors.margins: 10
                z: 1000
            }
        }
    }

    MouseArea {
        id: mouseArea
        property real lastMouseX: 0
        property real lastMouseY: 0
        property int activeEdges: 0
        property int gutterSize: 8
        property bool moveable: false
        hoverEnabled: true
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton

        cursorShape: {
            const p = Qt.point(mouseX, mouseY)
            if (p.x < gutterSize && p.y < gutterSize)
                return Qt.SizeFDiagCursor
            if (p.x >= width - gutterSize && p.y >= height - gutterSize)
                return Qt.SizeFDiagCursor
            if (p.x >= width - gutterSize && p.y < gutterSize)
                return Qt.SizeBDiagCursor
            if (p.x < gutterSize && p.y >= height - gutterSize)
                return Qt.SizeBDiagCursor
            if (p.x < gutterSize || p.x >= width - gutterSize)
                return Qt.SizeHorCursor
            if (p.y < gutterSize || p.y >= height - gutterSize)
                return Qt.SizeVerCursor
        }

        onDoubleClicked: {
            if (mainWindow.visibility == Window.Maximized) {
                mainWindow.showNormal()
            } else {
                mainWindow.showMaximized()
            }
            moveable = false
        }

        onPressed: {
            if (mouse.button !== Qt.LeftButton
                    || mainWindow.visibility === Window.Maximized
                    || mainWindow.visibility === Window.FullScreen) {
                return
            }
            activeEdges = 0
            if (mouseX < gutterSize)
                activeEdges |= Qt.LeftEdge
            if (mouseY < gutterSize)
                activeEdges |= Qt.TopEdge
            if (mouseX > mainWindow.width - gutterSize)
                activeEdges |= Qt.RightEdge
            if (mouseY > mainWindow.height - gutterSize)
                activeEdges |= Qt.BottomEdge

            if (mainWindow.startSystemMove != undefined
                    && Qt.platform.os !== "osx") {
                if (activeEdges == 0) {
                    mainWindow.startSystemMove()
                } else {
                    mainWindow.startSystemResize(activeEdges)
                }
            } else {
                lastMouseX = mouseX
                lastMouseY = mouseY
                moveable = (activeEdges === 0)
            }
        }

        onReleased: {
            activeEdges = 0
            moveable = false
        }

        onMouseXChanged: {
            // Use system native move & resize on Qt >= 5.15
            if (mainWindow.startSystemMove != undefined
                    && Qt.platform.os !== "osx") {
                return
            }

            if (mainWindow.visibility == Window.Maximized
                    || mainWindow.visibility == Window.FullScreen || !pressed) {
                return
            }

            if (activeEdges & Qt.LeftEdge) {
                mainWindow.width -= (mouseX - lastMouseX)
                if (mainWindow.width < window.minimumWidth) {
                    mainWindow.width = window.minimumWidth
                } else {
                    mainWindow.x += (mouseX - lastMouseX)
                }
            } else if (activeEdges & Qt.RightEdge) {
                mainWindow.width += (mouseX - lastMouseX)
                if (mainWindow.width < window.minimumWidth) {
                    mainWindow.width = window.minimumWidth
                }
                lastMouseX = mouseX
            } else if (moveable) {
                mainWindow.x += (mouseX - lastMouseX)
            }
        }

        onMouseYChanged: {
            // Use system native move & resize on Qt >= 5.15
            if (mainWindow.startSystemMove != undefined
                    && Qt.platform.os !== "osx") {
                return
            }

            if (mainWindow.visibility == Window.Maximized
                    || mainWindow.visibility == Window.FullScreen || !pressed) {
                return
            }

            if (activeEdges & Qt.TopEdge) {
                mainWindow.height -= (mouseY - lastMouseY)
                if (mainWindow.height < window.minimumHeight) {
                    mainWindow.height = window.minimumHeight
                } else {
                    mainWindow.y += (mouseY - lastMouseY)
                }
            } else if (activeEdges & Qt.BottomEdge) {
                mainWindow.height += (mouseY - lastMouseY)
                if (mainWindow.height < window.minimumHeight) {
                    mainWindow.height = window.minimumHeight
                }
                lastMouseY = mouseY
            } else if (moveable) {
                mainWindow.y += (mouseY - lastMouseY)
            }
        }
    }
}
