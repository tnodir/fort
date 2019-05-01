import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13

RowLayout {

    readonly property alias field: field
    readonly property alias combo: combo

    property real fieldPreferredWidth

    property var names: values
    property var values

    SpinBoxControl {
        id: field
        Layout.fillWidth: true
        Layout.preferredWidth: fieldPreferredWidth
        Layout.minimumWidth: fieldPreferredWidth

        onValueChanged: {
            combo.updateIndex(value);
        }
    }

    ComboBox {
        id: combo
        Layout.fillWidth: true

        model: names

        function getIndexByValue(value) {
            for (var i = values.length; --i >= 0; ) {
                if (value === values[i]) {
                    return i;
                }
            }
            return 0;
        }

        function updateIndex(value) {
            currentIndex = getIndexByValue(value);
        }

        onModelChanged: {
            updateIndex(field.value);
        }
        onActivated: {
            field.value = values[index];
        }
    }
}
