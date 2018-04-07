import QtQuick 2.12
import QtQuick.Controls 2.5

Frame {
    id: frame

    signal textChanged()  // Workaround for QTBUG-59908

    readonly property alias textArea: textArea

    padding: 0

    ScrollView {
        anchors.fill: parent

        TextArea {
            id: textArea
            clip: true  // to clip placeholder text
            persistentSelection: true
            selectByMouse: true
            onTextChanged: frame.textChanged()
            onReleased: textContextMenu.show(event, textArea)
        }
    }
}
