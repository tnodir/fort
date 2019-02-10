import QtQuick 2.12
import QtQuick.Controls 2.12

Pane {
    id: page

    bottomPadding: 0

    function onEditResetted() {
    }

    function onAboutToSave() {
    }

    function onSaved() {
    }

    Connections {
        target: mainPage
        onEditResetted: page.onEditResetted()
        onAboutToSave: page.onAboutToSave()
        onSaved: page.onSaved()
    }
}
