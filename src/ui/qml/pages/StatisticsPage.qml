import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../controls"
import com.fortfirewall 1.0

BasePage {

    readonly property LogManager logManager: fortManager.logManager

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
        }
    }
}
