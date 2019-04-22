import QtQuick 2.12
import QtQuick.Controls 2.12

TabBar {
    id: tabBar

    Rectangle {
        parent: tabBar.currentItem
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        color: palette.highlight
    }
}
