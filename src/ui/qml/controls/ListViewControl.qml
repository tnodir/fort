import QtQuick 2.12
import QtQuick.Controls 2.12

ListView {
    id: listView

    signal clicked(int index)

    readonly property bool hasCurrentItem:
        currentIndex >= 0 && currentIndex < count && !!currentItem

    Keys.onUpPressed: decrementCurrentIndex()
    Keys.onDownPressed: incrementCurrentIndex()

    ScrollBar.vertical: ScrollBarControl {}

    highlightResizeDuration: 0
    highlightMoveDuration: 200

    highlight: Item {
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: -8
            width: 3
            color: palette.highlight
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            const mouseX = listView.contentX + mouse.x;
            const mouseY = listView.contentY + mouse.y;

            var index = listView.indexAt(mouseX, mouseY);
            if (index < 0) {
                index = listView.indexAt(mouseX, mouseY - listView.spacing);
            }

            if (index >= 0) {
                listView.currentIndex = index;
                listView.clicked(index);
            }
        }
    }
}
