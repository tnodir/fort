import QtQuick 2.12
import QtQuick.Controls 2.5

ListView {
    id: listView

    signal clicked(int index)

    readonly property string currentItemText:
        (currentIndex >= 0 && currentIndex < count && currentItem)
        ? currentItem.displayText : ""

    Keys.onUpPressed: decrementCurrentIndex()
    Keys.onDownPressed: incrementCurrentIndex()

    ScrollBar.vertical: ScrollBarControl {}

    highlightResizeDuration: 0
    highlightMoveDuration: 200

    highlight: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1 - listView.spacing / 2 - border.width
            radius: 2
            border.width: 3
            border.color: palette.highlight
            color: "transparent"
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
