import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import "../Components"
import "../Controls"

Window {
    id: importIcaoFpl
    title: "Import ICAO FPL"
    width: 400
    height: 300
    minimumHeight: height
    minimumWidth: width
    maximumHeight: height
    maximumWidth: width
    modality: Qt.ApplicationModal
    flags: Qt.Dialog

    signal importFlightPlan(string flightPlan)

    GridLayout {
        anchors.fill: parent
        anchors.topMargin: 5
        anchors.bottomMargin: 15
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        columns: 1
        rows: 2

        Item {
            Layout.preferredHeight: 300
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.column: 0
            Layout.row: 0

            TextArea {
                id: txtFlightPlan
                height: 240
                width: 370
                Layout.column: 0
                Layout.row: 0
                onTextChanged: {
                    text = text.toUpperCase()
                    txtFlightPlan.cursorPosition = txtFlightPlan.text.length
                }
                background: Rectangle {
                    border.color: txtFlightPlan.focus ? '#565E64' : '#babfc4'
                    color: txtFlightPlan.enabled ? 'transparent' : '#E6E7E8'
                }
                font.pixelSize: 14
                font.family: ubuntuRegular.name
                color: '#333333'
                selectionColor: "#0164AD"
                selectedTextColor: "#ffffff"
                selectByMouse: true
                renderType: Text.NativeRendering
                wrapMode: TextEdit.WordWrap
                placeholderTextColor: "#babfc4"
                placeholderText: qsTr("(FPL-DCM311-IS
-B737/M-SDE2E3FGHIRWXY/LB1
-KBUR1445
-N0471F290 SLAPP2 MISEN NTNDO1
-KHND0050 KONT
-PBN/A1B1C1D1S1S2 NAV/RNVD1E2A1 REG/N737AT)")
            }
        }

        Item {
            Layout.preferredHeight: 30
            Layout.fillHeight: true
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true

            BlueButton {
                id: btnImportFpl
                text: qsTr("Import FPL")
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottomMargin: 0
                anchors.topMargin: 0
                anchors.rightMargin: 0
                anchors.leftMargin: 0
                font.pixelSize: 13
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        importFlightPlan(txtFlightPlan.text)
                    }
                }
            }
        }
    }
}
