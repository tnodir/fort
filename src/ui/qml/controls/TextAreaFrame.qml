import QtQuick 2.12
import QtQuick.Controls 2.12

Frame {
    id: frame

    signal textChanged()  // Workaround for QTBUG-59908

    readonly property alias textArea: textArea

    padding: 0

    ScrollView {
        anchors.fill: parent

        contentHeight: textArea.implicitHeight  // Workaround for QTBUG-72536

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
