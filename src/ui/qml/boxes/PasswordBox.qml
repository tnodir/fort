import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"
import com.fortfirewall 1.0

Pane {
    id: passwordBox

    enabled: false
    opacity: enabled ? 1.0 : 0

    Behavior on opacity { OpacityAnimator { duration: 200 } }

    property alias password: editPassword.text

    function reset() {
        enabled = firewallConf.hasPassword;
        password = "";
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: 400

        RowLayout {
            Layout.fillWidth: true

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Password:")
            }
            TextFieldFrame {
                id: editPassword
                Layout.fillWidth: true
                echoMode: TextInput.Password
                passwordMaskDelay: 300
            }
        }

        RowLayout {
            anchors.right: parent.right

            Button {
                icon.source: "qrc:/images/tick.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "OK")
                onClicked: {
                    if (stringUtil.cryptoHash(password)
                            === firewallConf.passwordHash) {
                        passwordBox.enabled = false;
                    } else {
                        editPassword.forceActiveFocus();
                        editPassword.selectAll();
                    }
                }
            }
            Button {
                icon.source: "qrc:/images/cancel.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Cancel")
                onClicked: closeWindow()
            }
        }
    }
}
