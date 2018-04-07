import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import "../controls"
import "log"
import com.fortfirewall 1.0

BasePage {

    readonly property LogManager logManager: fortManager.logManager
    readonly property AppBlockedModel appBlockedModel: logManager.appBlockedModel
    readonly property IpListModel ipListModel:
        appBlockedModel.ipListModel(currentAppPath)

    readonly property string currentAppPath: appListView.currentItemText

    readonly property string currentIpText: ipListView.currentItemText

    HostInfoCache {
        id: hostInfoCache
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            ButtonMenu {
                enabled: appListView.count
                icon.source: "qrc:/images/bin_empty.png"
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Clear…")

                MenuItem {
                    enabled: appListView.currentIndex >= 0
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Remove Application")
                    onTriggered: appBlockedModel.remove(
                                     appListView.currentIndex)
                }
                MenuItem {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Clear All")
                    onTriggered: {
                        appListView.currentIndex = -1;
                        appBlockedModel.clear();
                    }
                }
            }

            ButtonMenu {
                enabled: appListView.count
                icon.source: "qrc:/images/page_copy.png"
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Copy…")

                MenuItem {
                    enabled: !!currentAppPath
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Application Path")
                    onTriggered: guiUtil.setClipboardData(currentAppPath)
                }
                MenuItem {
                    enabled: !!currentIpText
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "IP Address")
                    onTriggered: guiUtil.setClipboardData(currentIpText)
                }
            }

            CheckBox {
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Resolve Addresses")
                checked: firewallConf.resolveAddress
                onToggled: {
                    if (firewallConf.resolveAddress === checked)
                        return;

                    firewallConf.resolveAddress = checked;

                    fortManager.applyConfImmediateFlags();

                    hostInfoCache.cacheChanged();  // refresh ipListView
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                font.weight: Font.DemiBold
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Show Blocked Applications")
                checked: firewallConf.logBlocked
                onToggled: {
                    if (firewallConf.logBlocked === checked)
                        return;

                    firewallConf.logBlocked = checked;

                    fortManager.applyConfImmediateFlags();
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            RowLayout {
                anchors.fill: parent
                spacing: 20

                AppListView {
                    id: appListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    model: appBlockedModel
                }

                IpListView {
                    id: ipListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    model: ipListModel
                }
            }
        }

        TextFieldFrame {
            Layout.fillWidth: true
            text: currentAppPath || ""
        }
    }
}
