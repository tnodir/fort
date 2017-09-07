import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

Page {
    id: mainPage

    signal opened()
    signal closed()
    signal saved()

    property bool confFlagsEdited
    property bool confEdited

    function setConfFlagsEdited() {
        confFlagsEdited = true;
    }

    function setConfEdited() {
        confEdited = true;
    }

    function resetConfEdited() {
        confFlagsEdited = false;
        confEdited = false;
    }

    onOpened: {
        resetConfEdited();

        tabBar.currentItem.forceActiveFocus();
    }

    header: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex

        TabButton {
            icon.source: "qrc:/images/cog.png"
            text: QT_TRANSLATE_NOOP("qml", "Options")
        }
        TabButton {
            icon.source: "qrc:/images/link.png"
            text: QT_TRANSLATE_NOOP("qml", "IPv4 Addresses")
        }
        TabButton {
            icon.source: "qrc:/images/application_cascade.png"
            text: QT_TRANSLATE_NOOP("qml", "Applications")
        }
        TabButton {
            icon.source: "qrc:/images/zoom.png"
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
                enabled: confFlagsEdited || confEdited
                text: QT_TRANSLATE_NOOP("qml", "OK")
                onClicked: {
                    if (fortManager.saveConf(confFlagsEdited)) {
                        mainPage.saved();
                        closeWindow();
                    }
                }
            }
            Button {
                enabled: confFlagsEdited || confEdited
                text: QT_TRANSLATE_NOOP("qml", "Apply")
                onClicked: {
                    if (fortManager.applyConf(confFlagsEdited)) {
                        resetConfEdited();
                    }
                }
            }
            Button {
                text: QT_TRANSLATE_NOOP("qml", "Cancel")
                onClicked: closeWindow()
            }
            Button {
                text: QT_TRANSLATE_NOOP("qml", "Quit")
                onClicked: fortManager.exit()
            }
        }
    }
}
