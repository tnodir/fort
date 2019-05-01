import QtQuick 2.12
import QtQuick.Controls 2.12

Label {
    id: bt

    opacity: enabled ? 1.0 : 0.45
    color: checked ? "darkblue" : (hovered ? "purple" : "blue")

    font.underline: true

    signal clicked()

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
