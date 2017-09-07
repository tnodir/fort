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
            addressGroup: firewallConf.ipInclude

            title {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Include")
            }
            checkBoxAll {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Include All")
            }
        }

        AddressesColumn {
            addressGroup: firewallConf.ipExclude

            title {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Exclude")
            }
            checkBoxAll {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Exclude All")
            }
        }
    }
}
