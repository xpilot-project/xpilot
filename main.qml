import QtQuick 2.15
import QtQuick.Controls 2.12
import QtQuick.Window 2.15
import QtQuick.Layouts 1.12
import QtWebSockets 1.1
import "./Resources/Components"

ApplicationWindow {
    property bool isModeC: false

    id: window
    title: "xPilot"
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    width: 800
    height: 250
    minimumHeight: 250
    minimumWidth: 800
    color: "#272C2E"

    WebSocket {
        id: socket
        url: "ws://localhost:9000/ws"
        active: true

        onBinaryMessageReceived: {
            console.log(message)
        }

        onTextMessageReceived: {
            console.log(message)
        }

        onStatusChanged: {
            switch(socket.status) {
            case WebSocket.Connecting:
                console.log('connecting...')
                break;
            case WebSocket.Open:
                console.log('open')
                break;
            case WebSocket.Closing:
                console.log('closing')
                break;
            case WebSocket.Closed:
                console.log('closed')
                break;
            case WebSocket.Error:
                console.log('Error: ' + errorString)
                break;
            }
        }
    }

    FontLoader {
        id: ubuntuRegular
        source: "./Resources/Fonts/Ubuntu-Regular.ttf"
    }

    FontLoader {
        id: robotoMono
        source: "./Resources/Fonts/Roboto-Mono.ttf"
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

            Toolbar {

            }
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
            id: messages
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 1
            color: "#141618"
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
            if (window.visibility == Window.Maximized) {
                window.showNormal()
            } else {
                window.showMaximized()
            }
            moveable = false
        }

        onPressed: {
            if (mouse.button !== Qt.LeftButton
                    || window.visibility === Window.Maximized
                    || window.visibility === Window.FullScreen) {
                return
            }
            activeEdges = 0
            if (mouseX < gutterSize)
                activeEdges |= Qt.LeftEdge
            if (mouseY < gutterSize)
                activeEdges |= Qt.TopEdge
            if (mouseX > window.width - gutterSize)
                activeEdges |= Qt.RightEdge
            if (mouseY > window.height - gutterSize)
                activeEdges |= Qt.BottomEdge

            if (window.startSystemMove !== undefined
                    && Qt.platform.os !== "osx") {
                if (activeEdges == 0) {
                    window.startSystemMove()
                } else {
                    window.startSystemResize(activeEdges)
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
            if (window.startSystemMove !== undefined
                    && Qt.platform.os !== "osx") {
                return
            }

            if (window.visibility == Window.Maximized
                    || window.visibility == Window.FullScreen || !pressed) {
                return
            }

            if (activeEdges & Qt.LeftEdge) {
                window.width -= (mouseX - lastMouseX)
                if (window.width < window.minimumWidth) {
                    window.width = window.minimumWidth
                } else {
                    window.x += (mouseX - lastMouseX)
                }
            } else if (activeEdges & Qt.RightEdge) {
                window.width += (mouseX - lastMouseX)
                if (window.width < window.minimumWidth) {
                    window.width = window.minimumWidth
                }
                lastMouseX = mouseX
            } else if (moveable) {
                window.x += (mouseX - lastMouseX)
            }
        }

        onMouseYChanged: {
            // Use system native move & resize on Qt >= 5.15
            if (window.startSystemMove !== undefined
                    && Qt.platform.os !== "osx") {
                return
            }

            if (window.visibility == Window.Maximized
                    || window.visibility == Window.FullScreen || !pressed) {
                return
            }

            if (activeEdges & Qt.TopEdge) {
                window.height -= (mouseY - lastMouseY)
                if (window.height < window.minimumHeight) {
                    window.height = window.minimumHeight
                } else {
                    window.y += (mouseY - lastMouseY)
                }
            } else if (activeEdges & Qt.BottomEdge) {
                window.height += (mouseY - lastMouseY)
                if (window.height < window.minimumHeight) {
                    window.height = window.minimumHeight
                }
                lastMouseY = mouseY
            } else if (moveable) {
                window.y += (mouseY - lastMouseY)
            }
        }
    }
}
