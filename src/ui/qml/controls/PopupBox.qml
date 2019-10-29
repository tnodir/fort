import QtQuick 2.13
import QtQuick.Controls 2.13

Popup {
    y: button.height
    visible: button.checked

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                 | Popup.CloseOnPressOutsideParent

    property Item button: parent

    onClosed: {
        button.checked = false;
    }
}
