import QtQuick 2.11
import QtQuick.Controls 2.4

Button {
    id: bt

    checkable: true

    default property alias content: menu.contentData

    Menu {
        id: menu
        y: bt.height
        visible: bt.checked

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                     | Popup.CloseOnPressOutsideParent

        onClosed: {
            bt.checked = false;
        }
    }
}
