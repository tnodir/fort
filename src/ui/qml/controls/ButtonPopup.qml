import QtQuick 2.13
import QtQuick.Controls 2.13

Button {
    id: bt

    checkable: true

    default property alias content: popup.contentData

    PopupBox {
        id: popup
    }
}
