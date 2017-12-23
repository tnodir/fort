import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"
import com.fortfirewall 1.0

BasePage {

    function onSaved() {  // override
        fortSettings.startWithWindows = cbStart.checked;
    }

    Frame {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            CheckBox {
                id: cbStart
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Start with Windows")
                checked: fortSettings.startWithWindows
                onToggled: {
                    setConfFlagsEdited();
                }
            }

            CheckBox {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Block access to network when Fort Firewall is not running")
                checked: firewallConf.provBoot
                onToggled: {
                    firewallConf.provBoot = checked;

                    setConfFlagsEdited();
                }
            }

            CheckBox {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Filter Enabled")
                checked: firewallConf.filterEnabled
                onToggled: {
                    firewallConf.filterEnabled = checked;

                    setConfFlagsEdited();
                }
            }

            CheckBox {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Stop Traffic")
                checked: firewallConf.stopTraffic
                onToggled: {
                    firewallConf.stopTraffic = checked;

                    setConfFlagsEdited();
                }
            }

            Row {
                spacing: 4

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Language:")
                }
                ComboBox {
                    width: Math.max(implicitWidth, 180)
                    flat: true
                    currentIndex: translationManager.language
                    model: translationManager.naturalLabels
                    onActivated: fortManager.setLanguage(index)
                }
            }

            Item {
                Layout.fillHeight: true
            }

            Row {
                spacing: 4

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Profile:")
                }
                LinkButton {
                    text: fortSettings.profilePath
                    onClicked: Qt.openUrlExternally("file:///" + text)
                }
            }

            Row {
                spacing: 4

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Releases:")
                }
                LinkButton {
                    text: fortSettings.appUpdatesUrl
                    onClicked: Qt.openUrlExternally(text)
                }
            }
        }
    }
}
