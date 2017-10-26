import QtQuick 2.9
import QtQuick.Controls 2.2

TextField {
    id: textField
    persistentSelection: true
    // XXX: QTBUG-64048: mouse right click clears selected text
    onReleased: textContextMenu.show(event, textField)
}
