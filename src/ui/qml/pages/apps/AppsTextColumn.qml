import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../../controls"
import com.fortfirewall 1.0

ColumnLayout {
    id: container
    Layout.preferredWidth: 100
    Layout.fillWidth: true
    Layout.fillHeight: true

    signal editingFinished()  // Workaround for QTBUG-59908

    readonly property alias title: title
    readonly property alias textArea: textAreaFrame.textArea

    Label {
        id: title
    }

    TextAreaFrame {
        id: textAreaFrame
        Layout.fillWidth: true
        Layout.fillHeight: true

        onEditingFinished: container.editingFinished()
    }
}
