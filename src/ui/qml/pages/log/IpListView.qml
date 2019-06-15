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
        text: (hostName || ipAddress) + ipProtoPort
        font.italic: !!hostName

        readonly property string hostName:
            (firewallConf.resolveAddress
             && hostInfoCache.hostTrigger
             && hostInfoCache.hostName(ipAddress)) || ""

        readonly property var ipParts: {
            const pos = display.indexOf(',');
            const address = (pos < 0) ? display : display.substring(0, pos);
            const protoPort = (pos < 0) ? "" : display.substring(pos);

            return [address, protoPort];
        }

        readonly property string ipAddress: ipParts[0]
        readonly property string ipProtoPort: ipParts[1]
    }
}
