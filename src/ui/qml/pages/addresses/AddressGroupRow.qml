import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../../box"
import com.fortfirewall 1.0

TextArea2SplitBox {
    id: splitBox

    property AddressGroup addressGroup

    textArea1: includeAddresses.textArea
    textArea2: excludeAddresses.textArea

    textMoveAllFrom1To2: translationManager.trTrigger
                         && qsTranslate("qml", "Move All Lines to 'Exclude'")
    textMoveAllFrom2To1: translationManager.trTrigger
                         && qsTranslate("qml", "Move All Lines to 'Include'")
    textMoveSelectedFrom1To2: translationManager.trTrigger
                              && qsTranslate("qml", "Move Selected Lines to 'Exclude'")
    textMoveSelectedFrom2To1: translationManager.trTrigger
                              && qsTranslate("qml", "Move Selected Lines to 'Include'")

    settingsPropName: "windowAddrSplit"

    AddressesColumn {
        id: includeAddresses
        SplitView.preferredWidth: splitBox.textArea1Width
        SplitView.minimumWidth: 250

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
        SplitView.minimumWidth: 250

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
