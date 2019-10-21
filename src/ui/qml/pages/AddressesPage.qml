import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../controls"
import "addresses"
import com.fortfirewall 1.0

BasePage {

    ColumnLayout {
        anchors.fill: parent

        TabBarControl {
            id: barGroups
            Layout.fillWidth: true

            TabButton {
                icon.source: "qrc:/images/world.png"
                text: qsTranslate("qml", "Internet Addresses")
            }
            TabButton {
                icon.source: "qrc:/images/world_link.png"
                text: qsTranslate("qml", "Allowed Internet Addresses")
            }
        }

        HSeparator {}

        AddressGroupRow {
            Layout.fillWidth: true
            Layout.fillHeight: true

            addressGroup: firewallConf.addressGroups[barGroups.currentIndex]
        }
    }
}
