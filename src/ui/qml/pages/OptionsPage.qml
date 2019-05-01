import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../controls"
import com.fortfirewall 1.0

BasePage {

    property bool iniEdited

    function setIniEdited() {
        iniEdited = true;

        setOthersEdited();
    }

    function onEditResetted() {  // override
        iniEdited = false;
    }

    function onAboutToSave() {  // override
        const password = editPassword.text;
        if (password) {
            firewallConf.passwordHash = stringUtil.cryptoHash(password);
            editPassword.text = "";
        }
    }

    function onSaved() {  // override
        if (!iniEdited) return;

        fortSettings.startWithWindows = cbStart.checked;
        fortSettings.hotKeyEnabled = cbHotKeys.checked;
    }

    Frame {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: 10

                CheckBox {
                    id: cbStart
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Start with Windows")
                    checked: fortSettings.startWithWindows
                    onToggled: setIniEdited()
                }

                CheckBox {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Stop traffic when Fort Firewall is not running")
                    checked: firewallConf.provBoot
                    onToggled: {
                        firewallConf.provBoot = checked;

                        setConfFlagsEdited();
                    }
                }

                Row {
                    spacing: 20

                    CheckBox {
                        id: cbFilterEnabled
                        width: Math.max(implicitWidth, cbStopTraffic.implicitWidth)
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
                        id: cbStopTraffic
                        width: cbFilterEnabled.width
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
                    onToggled: setIniEdited()
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
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: 10

                Frame {
                    Layout.alignment: Qt.AlignHCenter

                    ColumnLayout {
                        spacing: 10

                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 5
                            Image {
                                source: driverManager.isDeviceOpened
                                        ? (!driverManager.errorMessage
                                           ? "qrc:/images/plugin.png"
                                           : "qrc:/images/plugin_error.png")
                                        : "qrc:/images/plugin_disabled.png"
                            }
                            Label {
                                font.weight: Font.Bold
                                text: translationManager.trTrigger
                                      && qsTranslate("qml", "Driver:")
                            }
                            Label {
                                width: Math.min(implicitWidth, 300)
                                wrapMode: Text.Wrap
                                text: translationManager.trTrigger
                                      && (driverManager.isDeviceOpened
                                          ? (driverManager.errorMessage
                                             || qsTranslate("qml", "Installed"))
                                          : qsTranslate("qml", "Not Installed"))
                            }
                        }

                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 10
                            DelayButtonControl {
                                text: translationManager.trTrigger
                                      && qsTranslate("qml", "Install")
                                onDelayClicked: fortManager.installDriver()
                            }
                            DelayButtonControl {
                                text: translationManager.trTrigger
                                      && qsTranslate("qml", "Remove")
                                onDelayClicked: fortManager.removeDriver()
                            }
                        }
                    }
                }
            }
        }
    }
}
