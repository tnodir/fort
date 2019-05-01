import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../../controls"
import com.fortfirewall 1.0

ColumnLayout {
    Layout.preferredWidth: 100
    Layout.fillWidth: true
    Layout.fillHeight: true

    signal useAllToggled(bool checked)
    signal ipTextEdited(string ipText)

    readonly property alias title: title
    readonly property alias checkBoxAll: checkBoxAll
    readonly property alias textArea: textAreaFrame.textArea

    property bool useAll
    property string ipText

    RowLayout {
        Label {
            Layout.fillWidth: true
            id: title
            font.weight: Font.DemiBold
        }
        CheckBox {
            id: checkBoxAll
            checked: useAll
            onToggled: {
                useAllToggled(checked);

                setConfFlagsEdited();
            }
        }
    }

    TextAreaFrame {
        id: textAreaFrame
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        textArea {
            placeholderText: netUtil.localIpv4Networks().join('\n')
            text: ipText
        }

        onTextChanged: {
            if (ipText === textArea.text)
                return;

            ipTextEdited(textArea.text);

            setConfEdited();
        }
    }
}
