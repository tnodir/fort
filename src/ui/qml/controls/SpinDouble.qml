import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

RowLayout {

    readonly property alias field1: field1
    readonly property alias field2: field2

    property real fieldPreferredWidth

    SpinBox {
        id: field1
        Layout.fillWidth: true
        Layout.preferredWidth: fieldPreferredWidth
        Layout.minimumWidth: fieldPreferredWidth

        editable: true
        from: 0
        to: 9999
    }

    SpinBox {
        id: field2
        Layout.fillWidth: true
        Layout.preferredWidth: fieldPreferredWidth
        Layout.minimumWidth: fieldPreferredWidth

        editable: true
        from: 0
        to: 9999
    }
}
