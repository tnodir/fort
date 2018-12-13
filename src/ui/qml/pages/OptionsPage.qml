import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import "../controls"
import com.fortfirewall 1.0

BasePage {

    function onAboutToSave() {  // override
        const password = editPassword.text;
        if (password) {
            firewallConf.passwordHash = stringUtil.cryptoHash(password);
            editPassword.text = "";
        }
    }

    function onSaved() {  // override
        fortSettings.startWithWindows = cbStart.checked;
        fortSettings.hotKeyEnabled = cbHotKeys.checked;
    }

    Frame {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            CheckBox {
                id: cbStart
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Start with Windows")
                checked: fortSettings.startWithWindows
                onToggled: {
                    setConfFlagsEdited();
                }
            }

            CheckBox {
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Block access to network when Fort Firewall is not running")
                checked: firewallConf.provBoot
                onToggled: {
                    firewallConf.provBoot = checked;

                    setConfFlagsEdited();
                }
            }

            Row {
                spacing: 20

                CheckBox {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Filter Enabled")
                    checked: firewallConf.filterEnabled
                    onToggled: {
                        firewallConf.filterEnabled = checked;

                        setConfFlagsEdited();
                    }
                }

                CheckBox {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Filter Local Addresses")
                    checked: firewallConf.filterLocals
                    onToggled: {
                        firewallConf.filterLocals = checked;

                        setConfFlagsEdited();
                    }
                }
            }

            Row {
                spacing: 20

                CheckBox {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Stop Traffic")
                    checked: firewallConf.stopTraffic
                    onToggled: {
                        firewallConf.stopTraffic = checked;

                        setConfFlagsEdited();
                    }
                }

                CheckBox {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Stop Internet Traffic")
                    checked: firewallConf.stopInetTraffic
                    onToggled: {
                        firewallConf.stopInetTraffic = checked;

                        setConfFlagsEdited();
                    }
                }
            }

            CheckBox {
                id: cbHotKeys
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Hot Keys")
                checked: fortSettings.hotKeyEnabled
                onToggled: {
                    setConfFlagsEdited();
                }
            }

            Row {
                spacing: 4

                CheckBox {
                    id: cbPassword
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Password:")
                    checked: firewallConf.hasPassword
                    onToggled: {
                        if (!checked) {
                            firewallConf.passwordHash =
                                    editPassword.text = "";
                        } else {
                            editPassword.forceActiveFocus();
                        }

                        setConfEdited();
                    }
                }
                TextFieldFrame {
                    id: editPassword
                    width: 180
                    echoMode: TextInput.Password
                    passwordMaskDelay: 300
                    readOnly: firewallConf.hasPassword || !cbPassword.checked
                    placeholderText: translationManager.trTrigger
                                     && (firewallConf.hasPassword
                                         ? qsTranslate("qml", "Installed")
                                         : qsTranslate("qml", "Not Installed"))
                }
            }

            Row {
                spacing: 4

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: translationManager.trTrigger
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

            RowLayout {
                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Logs")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.logsPath
                }

                VSeparator {}

                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Profile")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.profilePath
                }

                VSeparator {}

                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Statistics")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.statPath
                }

                VSeparator {}

                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Releases")
                    tipText: link
                    onClicked: Qt.openUrlExternally(link)
                    readonly property string link: fortSettings.appUpdatesUrl
                }
            }
        }
    }
}
