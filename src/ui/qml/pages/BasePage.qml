import QtQuick 2.11
import QtQuick.Controls 2.4

Pane {
    id: page

    bottomPadding: 0

    function onAboutToSave() {
    }

    function onSaved() {
    }

    Connections {
        target: mainPage
        onAboutToSave: page.onAboutToSave()
        onSaved: page.onSaved()
    }
}
