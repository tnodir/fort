import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import com.fortfirewall 1.0

RowLayout {

    Layout.fillWidth: true

    readonly property alias checkBox: checkBox
    readonly property alias field: spinCombo.field
    readonly property alias combo: spinCombo.combo

    property alias names: spinCombo.names
    property alias values: spinCombo.values

    CheckBox {
        id: checkBox
        Layout.fillWidth: true
    }

    SpinCombo {
        id: spinCombo
        Layout.maximumWidth: implicitWidth

        fieldPreferredWidth: 140
    }
}
