import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../../controls"
import com.fortfirewall 1.0

ListViewControlTip {
    id: appListView

    spacing: 10

    property string emptyText
    property string emptyIcon

    tipText: (hoveredItem && hoveredItem.label.truncated)
             ? hoveredItem.label.text : ""

    onClicked: forceActiveFocus()

    delegate: RowLayout {
        id: appItem
        width: appListView.width
        spacing: 6

        readonly property alias label: label

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
            id: label
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
