import QtQuick 2.9
import QtQuick.Controls 2.2

Pane {
    id: page

    bottomPadding: 0

    function onSaved() {
    }

    Connections {
        target: mainPage
        onSaved: page.onSaved()
    }
}
