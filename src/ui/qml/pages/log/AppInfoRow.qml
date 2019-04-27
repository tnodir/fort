import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../../controls"
import com.fortfirewall 1.0

RowLayout {

    spacing: 10
    visible: !!appPath

    property string appPath

    readonly property var appInfo:
        appInfoCache.infoTrigger && appInfoCache.appInfo(appPath)

    RoundButtonTip {
        icon.source: "qrc:/images/page_copy.png"
        tipText: translationManager.trTrigger
                 && qsTranslate("qml", "Copy Path")
        onClicked: guiUtil.setClipboardData(appPath)
    }

    LinkButton {
        Layout.fillWidth: true
        elide: Text.ElideRight
        text: appPath
        onClicked: osUtil.openFolder(appPath)
    }

    Label {
        visible: !!appInfo.productName
        font.weight: Font.DemiBold
        text: appInfo.productName + " v" + appInfo.productVersion
    }
    Label {
        visible: !!appInfo.companyName
        text: appInfo.companyName
    }
}
