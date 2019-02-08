import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import com.fortfirewall 1.0

RowLayout {

    Layout.fillWidth: true

    readonly property alias checkBox: checkBox
    readonly property alias field1: spinDouble.field1
    readonly property alias field2: spinDouble.field2

    CheckBox {
        id: checkBox
        Layout.fillWidth: true
    }

    SpinDouble {
        id: spinDouble
        Layout.maximumWidth: implicitWidth

        fieldPreferredWidth: 140
    }
}
