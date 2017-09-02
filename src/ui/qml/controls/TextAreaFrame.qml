import QtQuick 2.9
import QtQuick.Controls 2.2

Frame {
    id: frame

    signal editingFinished()  // Workaround for QTBUG-59908

    readonly property alias textArea: textArea

    padding: 0

    ScrollView {
        anchors.fill: parent

        TextArea {
            id: textArea
            onEditingFinished: frame.editingFinished()
        }
    }
}
