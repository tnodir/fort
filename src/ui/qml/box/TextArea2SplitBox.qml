import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Templates 2.13 as T
import "../controls"
import com.fortfirewall 1.0

T.SplitView {
    id: control

    property TextArea textArea1
    property TextArea textArea2

    property string textMoveAllFrom1To2
    property string textMoveSelectedFrom1To2
    property string textMoveAllFrom2To1
    property string textMoveSelectedFrom2To1

    property string settingsPropName

    readonly property real handleWidth: 40

    property real splitOn: 0.5
    property bool splitOnEdited: false

    readonly property real textArea1Width: width * splitOn - handleWidth / 2

    function selectText(area, start, end) {
        area.forceActiveFocus();
        area.cursorPosition = start;
        area.moveCursorSelection(end);
    }

    function appendText(area, text) {
        if (!text) return;

        // Append the text to area
        const areaOldLen = area.length;
        area.append(text);

        // Select new text
        selectText(area, areaOldLen, area.length);
    }

    function moveAllLines(srcArea, dstArea) {
        // Cut the text from srcArea
        srcArea.selectAll();
        const text = srcArea.getText(srcArea.selectionStart, srcArea.selectionEnd);
        srcArea.clear();

        // Paste the text to dstArea
        appendText(dstArea, text);
    }

    function moveSelectedLines(srcArea, dstArea) {
        //if (!srcArea.has)

        // Cut the text from srcArea
        const srcText = srcArea.text;
        const srcStart = stringUtil.lineStart(srcText, srcArea.selectionStart) + 1;
        const srcEnd = stringUtil.lineEnd(srcText, srcArea.selectionEnd) + 1;

        const text = srcArea.getText(srcStart, srcEnd);
        srcArea.remove(srcStart, srcEnd);
        srcArea.deselect();

        console.log(">", srcStart, srcEnd, text.length);

        // Paste the text to dstArea
        appendText(dstArea, text);
    }

    Connections {
        target: fortManager
        onAfterSaveWindowState: {
            if (splitOnEdited) {
                fortSettings[settingsPropName] = splitOn;
            }
        }
        onAfterRestoreWindowState: {
            splitOn = fortSettings[settingsPropName] || 0.5;
        }
    }

    handle: Item {
        id: item
        implicitWidth: handleWidth
        implicitHeight: control.height

        readonly property bool pressed: T.SplitHandle.pressed

        onXChanged: {
            if (control.resizing) {
                splitOn = (x + handleWidth / 2) / control.width;
                splitOnEdited = true;
            }
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 10
            height: parent.height

            fillMode: Image.Tile
            source: "qrc:/images/stripes-light.png"

            Rectangle {
                anchors.fill: parent
                radius: 5
                border.width: 1
                border.color: item.pressed ? control.palette.midlight
                                           : "transparent"
                color: "transparent"
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 10

            RoundButtonTipSmall {
                icon.source: "qrc:/images/control_fastforward.png"
                tipText: textMoveAllFrom1To2
                onClicked: moveAllLines(textArea1, textArea2)
            }

            RoundButtonTipSmall {
                icon.source: "qrc:/images/control_play.png"
                tipText: textMoveSelectedFrom1To2
                onClicked: moveSelectedLines(textArea1, textArea2)
            }

            RoundButtonTipSmall {
                icon.source: "qrc:/images/control_rewind.png"
                tipText: textMoveAllFrom2To1
                onClicked: moveAllLines(textArea2, textArea1)
            }

            RoundButtonTipSmall {
                icon.source: "qrc:/images/control_play_backward.png"
                tipText: textMoveSelectedFrom2To1
                onClicked: moveSelectedLines(textArea2, textArea1)
            }
        }
    }
}
