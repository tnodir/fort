import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

ColumnLayout {
    Layout.preferredWidth: 100
    Layout.fillWidth: true
    Layout.fillHeight: true

    property alias title: title
    property alias checkBoxAll: checkBoxAll
    property alias textArea: textArea

    RowLayout {
        Label {
            Layout.fillWidth: true
            id: title
        }
        CheckBox {
            id: checkBoxAll
        }
    }

    Frame {
        Layout.fillWidth: true
        Layout.fillHeight: true

        padding: 0

        ScrollView {
            anchors.fill: parent

            TextArea {
                id: textArea
                enabled: !checkBoxAll.checked
            }
        }
    }
}
