import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

ColumnLayout {

    property int index
    property AppGroup appGroup

    RowLayout {
        Button {
            text: QT_TRANSLATE_NOOP("qml", "Remove Group")
            onClicked: removeAppGroup(index)
        }
        Button {
            text: QT_TRANSLATE_NOOP("qml", "Move left")
            onClicked: moveAppGroup(index, -1)
        }
        Button {
            text: QT_TRANSLATE_NOOP("qml", "Move right")
            onClicked: moveAppGroup(index, 1)
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            text: QT_TRANSLATE_NOOP("qml", "Enabled")
            checked: appGroup.enabled
            onToggled: {
                appGroup.enabled = checked;

                setConfFlagsEdited();
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 10

        AppsTextColumn {
            title {
                text: QT_TRANSLATE_NOOP("qml", "Block")
            }

            textArea {
                placeholderText: "
C:\\Program Files\\Internet Explorer\\iexplore.exe
"
                text: appGroup.blockText
            }

            onEditingFinished: {
                appGroup.blockText = textArea.text;

                setConfEdited();
            }
        }

        AppsTextColumn {
            title {
                text: QT_TRANSLATE_NOOP("qml", "Allow")
            }

            textArea {
                placeholderText: "
System
C:\\Program Files\\Skype\\Phone\\Skype.exe
"
                text: appGroup.allowText
            }

            onEditingFinished: {
                appGroup.allowText = textArea.text;

                setConfEdited();
            }
        }
    }
}
