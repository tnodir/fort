import QtQuick 2.13
import QtQuick.Controls 2.13
import "controls"
import "pages"
import com.fortfirewall 1.0

ApplicationWindow {
    id: appWindow

    width: 1025
    height: 768
    minimumWidth: 950
    minimumHeight: 600

    font.family: "Tahoma"
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

    DateUtil {
        id: dateUtil
    }

    FileUtil {
        id: fileUtil
    }

    GuiUtil {
        id: guiUtil
    }

    NetUtil {
        id: netUtil
    }

    OsUtil {
        id: osUtil
    }

    StringUtil {
        id: stringUtil
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
