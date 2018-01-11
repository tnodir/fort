import QtQuick 2.9
import QtQuick.Controls 2.2
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

            CheckBox {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Log Errors")
                checked: firewallConf.logErrors
                onToggled: {
                    firewallConf.logErrors = checked;

                    setConfFlagsEdited();
                }
            }

            Row {
                spacing: 4

                CheckBox {
                    id: cbPassword
                    text: translationManager.dummyBool
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
                    placeholderText: translationManager.dummyBool
                                     && (firewallConf.hasPassword
                                         ? qsTranslate("qml", "Installed")
                                         : qsTranslate("qml", "Not Installed"))
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

            RowLayout {
                LinkButton {
                    visible: firewallConf.logErrors
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Logs")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.logsPath
                }

                VSeparator {
                    visible: firewallConf.logErrors
                }

                LinkButton {
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Profile")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.profilePath
                }

                VSeparator {}

                LinkButton {
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Releases")
                    tipText: link
                    onClicked: Qt.openUrlExternally(link)
                    readonly property string link: fortSettings.appUpdatesUrl
                }
            }
        }
    }
}
