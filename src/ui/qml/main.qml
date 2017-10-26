import QtQuick 2.9
import QtQuick.Controls 2.2
import "controls"
import "pages"
import com.fortfirewall 1.0

ApplicationWindow {
    id: appWindow

    width: 1025
    height: 768
    minimumWidth: 700
    minimumHeight: 600

    font.pixelSize: 16

    readonly property FortSettings fortSettings: fortManager.fortSettings
    readonly property FirewallConf firewallConf: fortManager.firewallConfToEdit
                                                 || fortManager.firewallConf

    onClosing: {
        if (visible) {
            close.accepted = false;
            closeWindow();
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

    TextContextMenu {
        id: textContextMenu
    }

    Loader {
        anchors.fill: parent
        sourceComponent: appWindow.visible ? mainPageComponent : undefined
    }

    Component {
        id: mainPageComponent
        MainPage {
            Keys.onEscapePressed: closeWindow()

            Component.onCompleted: opened()
            Component.onDestruction: closed()
        }
    }
}
