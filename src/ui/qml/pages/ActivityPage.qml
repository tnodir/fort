import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    ColumnLayout {
        anchors.fill: parent

        Switch {
            anchors.right: parent.right
            onToggled: {
                firewallConf.appLogBlocked = checked;
            }
        }

        GridView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
