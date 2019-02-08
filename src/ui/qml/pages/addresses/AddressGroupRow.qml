import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import com.fortfirewall 1.0

RowLayout {

    spacing: 10

    property AddressGroup addressGroup

    AddressesColumn {
        id: includeAddresses

        title {
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Include")
        }
        checkBoxAll {
            enabled: !excludeAddresses.checkBoxAll.checked
                     || checkBoxAll.checked
            text: translationManager.trTrigger
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
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Exclude")
        }
        checkBoxAll {
            enabled: !includeAddresses.checkBoxAll.checked
                     || checkBoxAll.checked
            text: translationManager.trTrigger
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
