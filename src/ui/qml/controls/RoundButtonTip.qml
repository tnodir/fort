import QtQuick 2.12
import QtQuick.Controls 2.12

RoundButton {
    id: bt
    hoverEnabled: true

    property string tipText

    ToolTip.text: tipText
    ToolTip.visible: bt.hovered
    ToolTip.delay: 500
    ToolTip.timeout: 5000
}
