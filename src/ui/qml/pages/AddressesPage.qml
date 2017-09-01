import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "addresses"

BasePage {
    id: frame

    function pageFillConf(conf) {
        conf.ipIncludeAll = cbIncludeAll.checked;
        conf.ipExcludeAll = cbExcludeAll.checked;

        conf.ipIncludeText = taIncludeText.text;
        conf.ipExcludeText = taExcludeText.text;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 10

        AddressesColumn {
            title {
                text: QT_TRANSLATE_NOOP("qml", "Include")
            }
            checkBoxAll {
                id: cbIncludeAll
                text: QT_TRANSLATE_NOOP("qml", "Include All")
                checked: firewallConf.ipIncludeAll
            }
            textArea {
                id: taIncludeText
                text: firewallConf.ipIncludeText
            }
        }

        AddressesColumn {
            title {
                text: QT_TRANSLATE_NOOP("qml", "Exclude")
            }
            checkBoxAll {
                id: cbExcludeAll
                text: QT_TRANSLATE_NOOP("qml", "Exclude All")
                checked: firewallConf.ipExcludeAll
            }
            textArea {
                id: taExcludeText
                text: firewallConf.ipExcludeText
            }
        }
    }
}
