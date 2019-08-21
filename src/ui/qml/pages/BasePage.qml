import QtQuick 2.13
import QtQuick.Controls 2.13

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
        function onEditResetted() {
            page.onEditResetted();
        }
        function onAboutToSave() {
            page.onAboutToSave();
        }
        function onSaved() {
            page.onSaved();
        }
    }
}
