import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../../controls"
import com.fortfirewall 1.0

RowLayout {

    visible: !!appPath

    property string appPath

    TextFieldFrame {
        id: textField
        Layout.fillWidth: true
        text: appPath
    }

    RoundButtonTip {
        icon.source: "qrc:/images/page_copy.png"
        tipText: translationManager.trTrigger
                 && qsTranslate("qml", "Copy Path")
        onClicked: guiUtil.setClipboardData(appPath)
    }

    RoundButtonTip {
        icon.source: "qrc:/images/folder_go.png"
        tipText: translationManager.trTrigger
                 && qsTranslate("qml", "Open Folder")
        onClicked: osUtil.openFolder(appPath)
    }
}
