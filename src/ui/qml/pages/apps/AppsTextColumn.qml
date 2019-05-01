import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../../controls"
import com.fortfirewall 1.0

ColumnLayout {
    id: container
    Layout.fillWidth: true
    Layout.fillHeight: true

    signal textChanged()  // Workaround for QTBUG-59908

    readonly property alias title: title
    readonly property alias textArea: textAreaFrame.textArea

    Label {
        id: title
        font.weight: Font.DemiBold
    }

    TextAreaFrame {
        id: textAreaFrame
        Layout.preferredWidth: 100
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        onTextChanged: container.textChanged()
    }
}
