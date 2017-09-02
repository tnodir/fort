import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.2
import "pages"
import com.fortfirewall 1.0

ApplicationWindow {
    id: mainWindow

    visible: true
    title: QT_TRANSLATE_NOOP("qml", "Fort Firewall")

    width: 800
    height: 600
    minimumWidth: 400
    minimumHeight: 300

    font.pixelSize: 16

    readonly property FortSettings fortSettings: fortManager.fortSettings
    readonly property FirewallConf firewallConf: fortManager.firewallConfToEdit

    Component.onCompleted: {
        tabBar.currentItem.forceActiveFocus();
    }

    function closeWindow() {
        mainWindow.close();
    }

    Page {
        anchors.fill: parent

        Keys.onEscapePressed: closeWindow()

        header: TabBar {
            id: tabBar
            currentIndex: swipeView.currentIndex

            TabButton {
                text: QT_TRANSLATE_NOOP("qml", "Options")
            }
            TabButton {
                text: QT_TRANSLATE_NOOP("qml", "IPv4 Addresses")
            }
            TabButton {
                text: QT_TRANSLATE_NOOP("qml", "Applications")
            }
            TabButton {
                text: QT_TRANSLATE_NOOP("qml", "Activity")
            }
        }

        SwipeView {
            id: swipeView
            anchors.fill: parent
            currentIndex: tabBar.currentIndex

            OptionsPage {}
            AddressesPage {}
            ApplicationsPage {}
            ActivityPage {}
        }

        footer: Pane {
            RowLayout {
                anchors.right: parent.right

                Button {
                    text: QT_TRANSLATE_NOOP("qml", "OK")
                    onClicked: {
                        if (fortManager.saveConf())
                            closeWindow();
                    }
                }
                Button {
                    text: QT_TRANSLATE_NOOP("qml", "Cancel")
                    onClicked: closeWindow()
                }
                Button {
                    text: QT_TRANSLATE_NOOP("qml", "Quit")
                    onClicked: Qt.quit()
                }
            }
        }
    }
}
