import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"
import "log"
import com.fortfirewall 1.0

BasePage {

    readonly property LogManager logManager: fortManager.logManager
    readonly property AppBlockedModel appBlockedModel: logManager.appBlockedModel
    readonly property IpListModel ipListModel:
        appBlockedModel.ipListModel(currentAppPath)

    readonly property string currentAppPath:
        (appListView.currentIndex >= 0 && appListView.currentItem)
        ? appListView.currentItem.appPath : ""

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
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Clear")

                MenuItem {
                    enabled: appListView.currentIndex >= 0
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Remove Application")
                    onTriggered: appBlockedModel.remove(
                                     appListView.currentIndex)
                }
                MenuItem {
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Clear All")
                    onTriggered: {
                        appListView.currentIndex = -1;
                        appBlockedModel.clear();
                    }
                }
            }

            CheckBox {
                text: translationManager.dummyBool
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
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Log Blocked Applications")
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

                ListView {
                    id: ipListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 5

                    model: ipListModel

                    delegate: Label {
                        width: ipListView.width
                        elide: Text.ElideRight
                        text: (firewallConf.resolveAddress
                               && hostInfoCache.dummyBool
                               && hostInfoCache.hostName(ipText)) || ipText

                        readonly property string ipText: display
                    }

                    ScrollBar.vertical: ScrollBarControl {}
                }
            }
        }

        TextFieldFrame {
            Layout.fillWidth: true
            text: currentAppPath || ""
        }
    }
}
