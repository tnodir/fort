import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../../controls"
import com.fortfirewall 1.0

ListViewControlTip {
    id: appListView

    spacing: 10

    property string emptyText
    property string emptyIcon

    tipText: {
        var text = "";
        if (hoveredItem) {
            const appInfo = hoveredItem.appInfo;
            if (appInfo.productName) {
                text = appInfo.productName
                        + " v" + appInfo.productVersion
                        + "<br/><font color='gray'>("
                        + appInfo.companyName + ")</font>";
            }
        }
        return text;
    }

    onClicked: forceActiveFocus()

    delegate: RowLayout {
        id: appItem
        width: appListView.width
        spacing: 6

        readonly property bool isCurrent: ListView.isCurrentItem

        readonly property string appPath: display
        readonly property var appInfo:
            appInfoCache.infoTrigger && appInfoCache.appInfo(appPath)

        Image {
            Layout.topMargin: 1
            Layout.preferredWidth: iconWidth
            Layout.preferredHeight: iconWidth
            source: (!appItem.appPath && emptyIcon)
                    || appInfo.iconPath
                    || "qrc:/images/application.png"

            property int iconWidth: appItem.isCurrent ? 32 : 22

            Behavior on iconWidth { NumberAnimation { duration: 100 } }
        }
        Label {
            Layout.fillWidth: true
            color: appItem.isCurrent ? palette.highlight : palette.windowText
            font.pixelSize: 18
            elide: Text.ElideRight
            text: (!appItem.appPath && emptyText)
                  || appInfo.fileDescription
                  || fileUtil.fileName(appItem.appPath)
        }
    }
}
