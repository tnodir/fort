import QtQuick 2.13
import QtQuick.Controls 2.13

Label {
    id: bt

    opacity: enabled ? 1.0 : 0.45
    color: checked ? checkedColor : (hovered ? hoveredColor : normalColor)

    font.underline: true

    signal clicked()

    property color normalColor: "blue"
    property color hoveredColor: "purple"
    property color checkedColor: "darkblue"

    readonly property alias mouseArea: ma

    property bool selected: false
    property bool pressed: ma.pressed && ma.containsMouse
    readonly property bool checked: selected || pressed

    readonly property alias hovered: ma.containsMouse

    property string tipText

    ToolTip.text: tipText
    ToolTip.visible: bt.hovered && !!tipText
    ToolTip.delay: 500
    ToolTip.timeout: 5000

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: bt.clicked()
    }
}
