import QtQuick 2.12
import QtQuick.Controls 2.5
import "../../controls"
import com.fortfirewall 1.0

ListViewControl {
    id: ipListView

    spacing: 6

    delegate: Label {
        width: ipListView.width
        elide: Text.ElideRight
        text: (firewallConf.resolveAddress
               && hostInfoCache.hostTrigger
               && hostInfoCache.hostName(displayText)) || displayText

        readonly property string displayText: display
    }
}
