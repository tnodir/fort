import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../controls"
import "log"
import com.fortfirewall 1.0

BasePage {

    readonly property LogManager logManager: fortManager.logManager
    readonly property AppStatModel appStatModel: logManager.appStatModel

    readonly property string currentAppPath:
        (appListView.currentIndex >= 0 && appListView.currentItem)
        ? appListView.currentItem.appPath : ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            Switch {
                id: cbShowBlockedApps
                font.weight: Font.DemiBold
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Collect Traffic Statistics")
                checked: firewallConf.logStat
                onToggled: {
                    if (firewallConf.logStat === checked)
                        return;

                    firewallConf.logStat = checked;

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
                    Layout.preferredWidth: 100
                    Layout.fillHeight: true

                    model: appStatModel

                    emptyText: translationManager.dummyBool
                               && qsTranslate("qml", "All")
                }

            }
        }

        TextFieldFrame {
            Layout.fillWidth: true
            text: currentAppPath || ""
        }
    }
}
