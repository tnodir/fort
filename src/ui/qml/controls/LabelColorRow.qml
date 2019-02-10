import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import com.fortfirewall 1.0

RowLayout {

    Layout.fillWidth: true

    signal colorEdited()

    readonly property alias label: label
    readonly property alias button: button

    property real buttonPreferredWidth: 40

    property color defaultColor
    property color selectedColor: defaultColor

    Label {
        id: label
        Layout.fillWidth: true
    }

    Button {
        id: button
        Layout.fillWidth: true
        Layout.preferredWidth: buttonPreferredWidth
        Layout.minimumWidth: buttonPreferredWidth
        Layout.maximumWidth: implicitWidth
        Layout.preferredHeight: buttonPreferredWidth

        flat: true
        background: Rectangle {
            implicitWidth: buttonPreferredWidth
            implicitHeight: buttonPreferredWidth
            border.width: 1
            border.color: button.down ? "gray" : "black"
            color: selectedColor
        }

        onClicked: {
            const color = guiUtil.getColor(selectedColor);
            if (!guiUtil.isValidColor(color) || color === selectedColor)
                return;

            selectedColor = color;
            colorEdited();
        }
    }
}
