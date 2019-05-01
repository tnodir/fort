import QtQuick 2.13
import QtQuick.Controls 2.13

Menu {
    id: contextMenu

    property Item textField

    onClosed: {
        textField = null;
    }

    function show(mouseEvent, field) {
        if (mouseEvent.button !== Qt.RightButton)
            return;

        mouseEvent.accepted = true;

        textField = field;

        const pos = textField.mapToItem(null, mouseEvent.x, mouseEvent.y);
        x = pos.x;
        y = pos.y;

        open();
    }

    MenuItem {
        icon.source: "qrc:/images/page_copy.png"
        text: translationManager.trTrigger
              && qsTranslate("qml", "Copy")
        enabled: textField && textField.selectedText
        onTriggered: textField.copy()
    }
    MenuItem {
        icon.source: "qrc:/images/cut.png"
        text: translationManager.trTrigger
              && qsTranslate("qml", "Cut")
        enabled: textField && textField.selectedText
        onTriggered: textField.cut()
    }
    MenuItem {
        icon.source: "qrc:/images/page_paste.png"
        text: translationManager.trTrigger
              && qsTranslate("qml", "Paste")
        enabled: textField && textField.canPaste
        onTriggered: textField.paste()
    }

    MenuSeparator {}

    MenuItem {
        icon.source: "qrc:/images/textfield_rename.png"
        text: translationManager.trTrigger
              && qsTranslate("qml", "Select All")
        enabled: textField && textField.text
        onTriggered: textField.selectAll()
    }
}
