import QtQuick 2.11
import QtQuick.Controls 2.4

TextField {
    id: textField
    persistentSelection: true
    selectByMouse: true
    onReleased: textContextMenu.show(event, textField)
}
