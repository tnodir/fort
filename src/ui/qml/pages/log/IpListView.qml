import QtQuick 2.13
import QtQuick.Controls 2.13
import "../../controls"
import com.fortfirewall 1.0

ListViewControl {
    id: ipListView

    spacing: 6

    delegate: Label {
        width: ipListView.width
        elide: Text.ElideRight
        text: hostName || ipAddress
        font.italic: !!hostName

        readonly property string hostName:
            (firewallConf.resolveAddress
             && hostInfoCache.hostTrigger
             && hostInfoCache.hostName(ipAddress)) || ""

        readonly property string ipAddress: display
    }
}
