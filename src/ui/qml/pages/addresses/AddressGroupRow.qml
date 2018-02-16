import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.fortfirewall 1.0

RowLayout {

    spacing: 10

    property AddressGroup addressGroup

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
