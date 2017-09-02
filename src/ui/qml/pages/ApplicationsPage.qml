import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Button {
                text: QT_TRANSLATE_NOOP("qml", "Add Group")
            }
            Button {
                text: QT_TRANSLATE_NOOP("qml", "Remove Group")
            }

            Item {
                Layout.fillWidth: true
            }

            CheckBox {
                text: QT_TRANSLATE_NOOP("qml", "Block All")
            }
            CheckBox {
                text: QT_TRANSLATE_NOOP("qml", "Allow All")
            }
        }

        TabBar {
            id: barGroups
            Layout.fillWidth: true

            TabButton {
                text: QT_TRANSLATE_NOOP("qml", "Options")
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
