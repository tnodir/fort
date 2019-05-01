import QtQuick 2.13
import QtQuick.Controls 2.13

SpinBox {
    id: field

    editable: true
    from: 0
    to: 9999

    value: defaultValue

    signal valueEdited()

    property int defaultValue

    onValueChanged: {
        const value = field.value;
        if (value === defaultValue)
            return;

        field.valueEdited();
    }
}
