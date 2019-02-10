import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import com.fortfirewall 1.0

RowLayout {

    Layout.fillWidth: true

    readonly property alias label: label
    readonly property alias field: field

    property real fieldPreferredWidth: 140

    Label {
        id: label
        Layout.fillWidth: true
    }

    SpinBoxControl {
        id: field
        Layout.fillWidth: true
        Layout.preferredWidth: fieldPreferredWidth
        Layout.minimumWidth: fieldPreferredWidth
        Layout.maximumWidth: implicitWidth
    }
}
