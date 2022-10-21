import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Rectangle {
    id: root

    property double minimum: 0
    property double maximum: 1
    property double value: 0

    width: parent.width
    height: 15
    color: '#e6e7e8'
    radius: 20

    Rectangle {
        visible: value > minimum
        x: 0.1 * root.height
        y: 0.1 * root.height
        width: Math.max(height, Math.min((value - minimum) / (maximum - minimum) * (parent.width - 0.2 * root.height), parent.width - 0.2 * root.height))
        height: 0.8 * root.height
        color: (value < 0.70) ? '#0164AD' : (value >= 0.70 && value < 0.90) ? '#28A745' : '#DC3545'
        radius: parent.radius
    }
}
