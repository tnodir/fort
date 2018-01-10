import QtQuick 2.9
import QtQuick.Controls 2.2

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
