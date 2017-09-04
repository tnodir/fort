import QtQuick 2.9
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.2
import "pages"
import com.fortfirewall 1.0

ApplicationWindow {
    id: appWindow

    width: 800
    height: 600
    minimumWidth: 700
    minimumHeight: 600

    font.pixelSize: 16

    readonly property FortSettings fortSettings: fortManager.fortSettings
    readonly property FirewallConf firewallConf: fortManager.firewallConfToEdit

    onClosing: {
        if (visible) {
            close.accepted = false;
            closeWindow();
        }
    }

    onVisibleChanged: {
        if (visible) {
            mainPage.opened();
        } else {
            mainPage.closed();
        }
    }

    function closeWindow() {
        fortManager.closeWindow();
    }

    MainPage {
        id: mainPage
        anchors.fill: parent

        Keys.onEscapePressed: closeWindow()
    }
}
