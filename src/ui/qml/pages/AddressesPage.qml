import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"
import "addresses"
import com.fortfirewall 1.0

BasePage {

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: barGroups
            Layout.fillWidth: true

            TabButton {
                icon.source: "qrc:/images/world.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Internet")
            }
            TabButton {
                icon.source: "qrc:/images/world_link.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Filter Internet")
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
