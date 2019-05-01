import QtQuick 2.13
import QtQuick.Controls 2.13

TextField {
    id: textField
    persistentSelection: true
    selectByMouse: true
    onReleased: textContextMenu.show(event, textField)
}
