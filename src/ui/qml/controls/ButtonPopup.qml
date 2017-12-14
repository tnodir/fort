import QtQuick 2.9
import QtQuick.Controls 2.2

Button {
    id: bt

    checkable: true

    default property alias content: popup.contentData

    Popup {
        id: popup
        y: bt.height
        visible: bt.checked

        onClosed: {
            bt.checked = false;
        }
    }
}
