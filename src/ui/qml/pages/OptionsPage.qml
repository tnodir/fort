import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    function onSaved() {  // overload
        fortSettings.startWithWindows = cbStart.checked;
    }

    Column {
        anchors.fill: parent

        CheckBox {
            id: cbStart
            text: QT_TRANSLATE_NOOP("qml", "Start with Windows")
            checked: fortSettings.startWithWindows
        }
        CheckBox {
            id: cbFilter
            text: QT_TRANSLATE_NOOP("qml", "Filter Enabled")
            checked: firewallConf.filterEnabled
            onToggled: {
                firewallConf.filterEnabled = checked;
            }
        }
    }
}
