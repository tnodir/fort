import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "addresses"
import com.fortfirewall 1.0

BasePage {

    RowLayout {
        anchors.fill: parent
        spacing: 10

        AddressesColumn {
            id: includeAddresses

            addressGroup: firewallConf.ipInclude

            title {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Include")
            }
            checkBoxAll {
                enabled: !excludeAddresses.checkBoxAll.checked
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Include All")
            }
        }

        AddressesColumn {
            id: excludeAddresses

            addressGroup: firewallConf.ipExclude

            title {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Exclude")
            }
            checkBoxAll {
                enabled: !includeAddresses.checkBoxAll.checked
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Exclude All")
            }
        }
    }
}
