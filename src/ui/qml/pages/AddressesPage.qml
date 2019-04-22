import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
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
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Internet Addresses")
            }
            TabButton {
                icon.source: "qrc:/images/world_link.png"
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Allowed Internet Addresses")
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
