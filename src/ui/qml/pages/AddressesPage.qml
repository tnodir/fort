import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "addresses"
import com.fortfirewall 1.0

BasePage {

    property AddressGroup addressGroup: firewallConf.inetAddressGroup

    RowLayout {
        anchors.fill: parent
        spacing: 10

        AddressesColumn {
            id: includeAddresses

            title {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Include")
            }
            checkBoxAll {
                enabled: !excludeAddresses.checkBoxAll.checked
                         || checkBoxAll.checked
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Include All")
            }

            useAll: addressGroup.includeAll
            ipText: addressGroup.includeText

            onUseAllToggled: {
                addressGroup.includeAll = checked;
            }
            onIpTextEdited: {
                addressGroup.includeText = ipText;
            }
        }

        AddressesColumn {
            id: excludeAddresses

            title {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Exclude")
            }
            checkBoxAll {
                enabled: !includeAddresses.checkBoxAll.checked
                         || checkBoxAll.checked
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Exclude All")
            }

            useAll: addressGroup.excludeAll
            ipText: addressGroup.excludeText

            onUseAllToggled: {
                addressGroup.excludeAll = checked;
            }
            onIpTextEdited: {
                addressGroup.excludeText = ipText;
            }
        }
    }
}
