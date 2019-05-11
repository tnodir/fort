import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13

RowLayout {

    readonly property alias field1: field1
    readonly property alias field2: field2

    property real fieldPreferredWidth

    SpinTime {
        id: field1
        Layout.fillWidth: true
        Layout.preferredWidth: fieldPreferredWidth
        Layout.minimumWidth: fieldPreferredWidth
    }

    SpinTime {
        id: field2
        Layout.fillWidth: true
        Layout.preferredWidth: fieldPreferredWidth
        Layout.minimumWidth: fieldPreferredWidth
    }
}
