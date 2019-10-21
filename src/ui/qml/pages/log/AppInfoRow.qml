import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
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
        tipText: qsTranslate("qml", "Copy Path")
        onClicked: guiUtil.setClipboardData(appPath)
    }

    Item {
        Layout.fillWidth: true
        LinkButton {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, parent.width)
            elide: Text.ElideLeft
            tipText: qsTranslate("qml", "Open Folder")
                     + (truncated ? "<br/>" + text : "")
            text: appPath
            onClicked: osUtil.openFolder(appPath)
        }
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
