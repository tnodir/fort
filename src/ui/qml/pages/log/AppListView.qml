import QtQuick 2.9
import QtQuick.Controls 2.2
import "../../controls"
import com.fortfirewall 1.0

ListViewControl {
    id: appListView

    spacing: 10

    property string emptyText
    property string emptyIcon

    onClicked: forceActiveFocus()

    delegate: Row {
        id: appItem
        width: appListView.width
        spacing: 6

        readonly property string displayText: display

        // TODO: Use SHGetFileInfo() to get app's display name and icon
        Image {
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 1
            source: (!appItem.displayText && emptyIcon)
                    || "qrc:/images/application.png"
        }
        Label {
            font.pixelSize: 20
            elide: Text.ElideRight
            text: (!appItem.displayText && emptyText)
                  || fileUtil.fileName(appItem.displayText)
        }
    }
}
