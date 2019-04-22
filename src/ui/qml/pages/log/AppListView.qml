import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../../controls"
import com.fortfirewall 1.0

ListViewControl {
    id: appListView

    spacing: 10

    property string emptyText
    property string emptyIcon

    onClicked: forceActiveFocus()

    delegate: RowLayout {
        id: appItem
        width: appListView.width
        spacing: 6

        readonly property bool isCurrent: ListView.isCurrentItem
        readonly property int iconWidth: isCurrent ? 32 : 22

        readonly property string appPath: display
        readonly property var appInfo:
            appInfoCache.infoTrigger && appInfoCache.appInfo(appPath)

        Image {
            Layout.topMargin: 1
            Layout.preferredWidth: appItem.iconWidth
            Layout.preferredHeight: appItem.iconWidth
            source: (!appItem.appPath && emptyIcon)
                    || appInfo.iconPath
                    || "qrc:/images/application.png"
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
