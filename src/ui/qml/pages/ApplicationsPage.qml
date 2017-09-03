import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "apps"
import com.fortfirewall 1.0

BasePage {

    readonly property var appGroups: firewallConf.appGroups

    function resetGroupName() {
        editGroupName.text = "";
        editGroupName.forceActiveFocus();
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            TextField {
                enabled: rptAppGroups.count < 16
                id: editGroupName
                placeholderText: QT_TRANSLATE_NOOP("qml", "Group Name")
            }
            Button {
                enabled: editGroupName.text
                text: QT_TRANSLATE_NOOP("qml", "Add Group")
                onClicked: {
                    const lastIndex = appGroups.length;
                    firewallConf.addAppGroupByName(editGroupName.text);
                    barGroups.currentIndex = lastIndex;
                    resetGroupName();
                }
            }
            Button {
                enabled: editGroupName.text && appsColumn.enabled
                text: QT_TRANSLATE_NOOP("qml", "Rename Group")
                onClicked: {
                    const appGroup = appsColumn.appGroup;
                    appGroup.name = editGroupName.text;
                    resetGroupName();
                }
            }

            Item {
                Layout.fillWidth: true
            }

            CheckBox {
                text: QT_TRANSLATE_NOOP("qml", "Block All")
                checked: firewallConf.appBlockAll
                onToggled: {
                    firewallConf.appBlockAll = checked;
                }
            }
            CheckBox {
                text: QT_TRANSLATE_NOOP("qml", "Allow All")
                checked: firewallConf.appAllowAll
                onToggled: {
                    firewallConf.appAllowAll = checked;
                }
            }
        }

        TabBar {
            id: barGroups
            Layout.fillWidth: true
            clip: true

            Repeater {
                id: rptAppGroups
                model: appGroups

                TabButton {
                    width: Math.max(70, implicitWidth)
                    font.bold: checked
                    text: appGroup.name

                    readonly property AppGroup appGroup: modelData
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        AppsColumn {
            id: appsColumn
            Layout.fillWidth: true
            Layout.fillHeight: true

            enabled: index >= 0
            opacity: enabled ? 1.0 : 0

            Behavior on opacity { NumberAnimation { duration: 150 } }

            index: barGroups.currentIndex
            appGroup: enabled ? barGroups.currentItem.appGroup
                              : nullAppGroup

            readonly property AppGroup nullAppGroup: AppGroup {}
        }
    }
}
