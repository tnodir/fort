import QtQuick 2.9
import QtQuick.Controls 2.2

Frame {
    id: page

    function onSaved() {
    }

    Connections {
        target: mainPage
        onSaved: page.onSaved()
    }
}
