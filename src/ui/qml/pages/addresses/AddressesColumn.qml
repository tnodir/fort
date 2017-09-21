import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../../controls"
import com.fortfirewall 1.0

ColumnLayout {
    Layout.preferredWidth: 100
    Layout.fillWidth: true
    Layout.fillHeight: true

    readonly property alias title: title
    readonly property alias checkBoxAll: checkBoxAll

    property AddressGroup addressGroup

    RowLayout {
        Label {
            Layout.fillWidth: true
            id: title
            font.weight: Font.DemiBold
        }
        CheckBox {
            id: checkBoxAll
            checked: addressGroup.useAll
            onToggled: {
                addressGroup.useAll = checked;

                setConfFlagsEdited();
            }
        }
    }

    TextAreaFrame {
        Layout.fillWidth: true
        Layout.fillHeight: true

        textArea {
            placeholderText: "
10.0.0.0/8
127.0.0.0/8
169.254.0.0/16
172.16.0.0/12
192.168.0.0/16
"
            text: addressGroup.text
        }

        onTextChanged: {
            addressGroup.text = textArea.text;

            setConfEdited();
        }
    }
}
