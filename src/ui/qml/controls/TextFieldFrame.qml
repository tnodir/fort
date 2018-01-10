import QtQuick 2.9
import QtQuick.Controls 2.2

TextField {
    id: textField
    persistentSelection: true
    selectByMouse: true
    onReleased: textContextMenu.show(event, textField)
}
