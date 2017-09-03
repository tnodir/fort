import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

Page {
    id: mainPage

    signal saved()

    function initialize() {
        tabBar.currentItem.forceActiveFocus();
    }

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
                    if (fortManager.saveConf()) {
                        mainPage.saved();
                        closeWindow();
                    }
                }
            }
            Button {
                text: QT_TRANSLATE_NOOP("qml", "Apply")
                onClicked: fortManager.applyConf()
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
