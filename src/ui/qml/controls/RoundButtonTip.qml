import QtQuick 2.13
import QtQuick.Controls 2.13

RoundButton {
    id: bt
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    property string tipText

    ToolTip.text: tipText
    ToolTip.visible: bt.hovered
    ToolTip.delay: 500
    ToolTip.timeout: 5000
}
