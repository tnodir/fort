import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    ColumnLayout {
        CheckBox {
            text: QT_TRANSLATE_NOOP("qml", "Enabled")
            checked: firewallConf.appLogBlocked
            onToggled: {
                firewallConf.appLogBlocked = checked;
            }
        }
    }
}
