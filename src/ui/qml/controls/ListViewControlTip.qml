import QtQuick 2.12
import QtQuick.Controls 2.12

ListViewControl {

    property Item hoveredItem: null

    property alias tipText: toolTip.text

    mouseArea {
        hoverEnabled: true
        onPositionChanged: {
            hoveredItem = itemAtContent(mouse);
        }
        onExited: {
            hoveredItem = null;
        }
    }

    ToolTip {
        id: toolTip
        parent: hoveredItem
        visible: !!text
        delay: 500
        timeout: 5000
    }
}
