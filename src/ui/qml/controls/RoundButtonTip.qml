import QtQuick 2.11
import QtQuick.Controls 2.4

RoundButton {
    id: bt
    hoverEnabled: true

    property alias tipText: toolTip.text

    ToolTip {
        id: toolTip
        visible: bt.hovered
        delay: 500
        timeout: 5000
    }
}
