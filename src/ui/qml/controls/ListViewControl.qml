import QtQuick 2.13
import QtQuick.Controls 2.13

ListView {
    id: listView

    signal clicked(int index)

    readonly property alias mouseArea: mouseArea

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

    function itemAtContent(mouse) {
        const posX = listView.contentX + mouse.x;
        const posY = listView.contentY + mouse.y;

        var item = listView.itemAt(posX, posY);
        if (!item) {
            item = listView.itemAt(posX, posY - listView.spacing);
        }

        return item;
    }

    function indexAtContent(mouse) {
        const posX = listView.contentX + mouse.x;
        const posY = listView.contentY + mouse.y;

        var index = listView.indexAt(posX, posY);
        if (index < 0) {
            index = listView.indexAt(posX, posY - listView.spacing);
        }

        return index;
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            const index = indexAtContent(mouse);
            if (index >= 0) {
                listView.currentIndex = index;
                listView.clicked(index);
            }
        }
    }
}
