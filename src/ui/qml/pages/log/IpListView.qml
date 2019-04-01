import QtQuick 2.12
import QtQuick.Controls 2.12
import "../../controls"
import com.fortfirewall 1.0

ListViewControl {
    id: ipListView

    spacing: 6

    delegate: Label {
        width: ipListView.width
        elide: Text.ElideRight
        text: hostName || displayText
        font.italic: !!hostName

        readonly property string hostName:
            (firewallConf.resolveAddress
             && hostInfoCache.hostName(displayText)) || ""

        readonly property string displayText: display
    }
}
