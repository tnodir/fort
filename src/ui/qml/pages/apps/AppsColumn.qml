import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../../box"
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

        GroupOptionsButton {
            enabled: firewallConf.logStat
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            id: cbEnabled
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Enabled")
            checked: appGroup.enabled
            onToggled: {
                appGroup.enabled = checked;

                setConfFlagsEdited();
            }
        }

        SpinDoubleRow {
            Layout.maximumWidth: implicitWidth
            enabled: cbEnabled.checked

            checkBox {
                text: translationManager.trTrigger
                      && qsTranslate("qml", "period, hours:")
                checked: appGroup.periodEnabled
                onToggled: {
                    appGroup.periodEnabled = checkBox.checked;

                    setConfEdited();
                }
            }
            field1 {
                from: 0
                to: 24
                defaultValue: appGroup.periodFrom
                onValueEdited: {
                    appGroup.periodFrom = field1.value;

                    setConfEdited();
                }
            }
            field2 {
                from: 0
                to: 24
                defaultValue: appGroup.periodTo
                onValueEdited: {
                    appGroup.periodTo = field2.value;

                    setConfEdited();
                }
            }
        }
    }

    TextArea2SplitBox {
        id: splitBox
        Layout.fillWidth: true
        Layout.fillHeight: true

        textArea1: blockApps.textArea
        textArea2: allowApps.textArea

        textMoveAllFrom1To2: translationManager.trTrigger
                             && qsTranslate("qml", "Move All Lines to 'Allow'")
        textMoveAllFrom2To1: translationManager.trTrigger
                             && qsTranslate("qml", "Move All Lines to 'Block'")
        textMoveSelectedFrom1To2: translationManager.trTrigger
                                  && qsTranslate("qml", "Move Selected Lines to 'Allow'")
        textMoveSelectedFrom2To1: translationManager.trTrigger
                                  && qsTranslate("qml", "Move Selected Lines to 'Block'")

        selectFileEnabled: true

        settingsPropName: "windowAppsSplit"

        AppsTextColumn {
            id: blockApps
            SplitView.preferredWidth: splitBox.textArea1Width
            SplitView.minimumWidth: 150

            title {
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Block")
            }

            textArea {
                text: appGroup.blockText
            }

            onTextChanged: {
                if (appGroup.blockText === textArea.text)
                    return;

                appGroup.blockText = textArea.text;

                setConfEdited();
            }
        }

        AppsTextColumn {
            id: allowApps
            SplitView.minimumWidth: 150

            title {
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Allow")
            }

            textArea {
                placeholderText:
                    translationManager.trTrigger && (
                        qsTranslate("qml", "# Examples:")
                        + "\nSystem"
                        + "\nC:\\Program Files (x86)\\Microsoft\\Skype for Desktop\\Skype.exe"
                        + "\n\n"
                        + qsTranslate("qml", "# All programs in the sub-path:")
                        + "\nC:\\Git\\"
                        )
                text: appGroup.allowText
            }

            onTextChanged: {
                if (appGroup.allowText === textArea.text)
                    return;

                appGroup.allowText = textArea.text;

                setConfEdited();
            }
        }
    }
}
