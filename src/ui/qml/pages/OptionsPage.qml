import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    function onSaved() {  // overload
        fortSettings.startWithWindows = cbStart.checked;
    }

    Frame {
        anchors.fill: parent

        Column {
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
                id: cbFilter
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Filter Enabled")
                checked: firewallConf.filterEnabled
                onToggled: {
                    firewallConf.filterEnabled = checked;

                    setConfFlagsEdited();
                }
            }
            ComboBox {
                width: Math.max(implicitWidth, 180)
                flat: true
                currentIndex: translationManager.language
                model: translationManager.getNaturalLabels()
                onActivated: fortManager.setLanguage(index)
            }
        }
    }
}
