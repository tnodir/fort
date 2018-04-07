import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import "../../controls"
import com.fortfirewall 1.0

ColumnLayout {

    property int index
    property AppGroup appGroup

    RowLayout {
        RoundButtonTip {
            icon.source: "qrc:/images/application_delete.png"
            tipText: translationManager.trTrigger
                     && qsTranslate("qml", "Remove Group")
            onClicked: removeAppGroup(index)
        }
        RoundButtonTip {
            icon.source: "qrc:/images/resultset_previous.png"
            tipText: translationManager.trTrigger
                     && qsTranslate("qml", "Move left")
            onClicked: moveAppGroup(index, -1)
        }
        RoundButtonTip {
            icon.source: "qrc:/images/resultset_next.png"
            tipText: translationManager.trTrigger
                     && qsTranslate("qml", "Move right")
            onClicked: moveAppGroup(index, 1)
        }

        Item {
            Layout.preferredWidth: 10
        }

        SpeedLimitButton {
            enabled: firewallConf.logStat
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Enabled")
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
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Block")
            }

            textArea {
                text: appGroup.blockText
            }

            onTextChanged: {
                if (appGroup.blockText == textArea.text)
                    return;

                appGroup.blockText = textArea.text;

                setConfEdited();
            }
        }

        AppsTextColumn {
            title {
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Allow")
            }

            textArea {
                placeholderText: "
System
C:\\Program Files (x86)\\Microsoft\\Skype for Desktop\\Skype.exe
"
                text: appGroup.allowText
            }

            onTextChanged: {
                if (appGroup.allowText == textArea.text)
                    return;

                appGroup.allowText = textArea.text;

                setConfEdited();
            }
        }
    }
}
