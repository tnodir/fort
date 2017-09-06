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

    FileUtil {
        id: fileUtil
    }

    NetUtil {
        id: netUtil
    }

    OsUtil {
        id: osUtil
    }

    MainPage {
        id: mainPage
        anchors.fill: parent
        implicitWidth: 0  // XXX: Workaround for binding loop

        Keys.onEscapePressed: closeWindow()
    }
}
