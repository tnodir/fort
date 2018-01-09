import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
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
            placeholderText: netUtil.localIpv4Networks().join('\n')
            text: addressGroup.text
        }

        onTextChanged: {
            if (addressGroup.text == textArea.text)
                return;

            addressGroup.text = textArea.text;

            setConfEdited();
        }
    }
}
