import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import com.fortfirewall 1.0

Page {
    id: mainPage

    signal opened()
    signal closed()
    signal saved()

    property bool confFlagsEdited
    property bool confEdited
    property bool scheduleEdited

    function setConfFlagsEdited() {
        confFlagsEdited = true;
    }

    function setConfEdited() {
        confEdited = true;
    }

    function setScheduleEdited() {
        scheduleEdited = true;
    }

    function resetEdited() {
        confFlagsEdited = false;
        confEdited = false;
        scheduleEdited = false;
    }

    onOpened: {
        resetEdited();

        tabBar.currentItem.forceActiveFocus();
    }

    header: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex

        TabButton {
            icon.source: "qrc:/images/cog.png"
            text: translationManager.dummyBool
                  && qsTranslate("qml", "Options")
        }
        TabButton {
            icon.source: "qrc:/images/link.png"
            text: translationManager.dummyBool
                  && qsTranslate("qml", "IPv4 Addresses")
        }
        TabButton {
            icon.source: "qrc:/images/application_double.png"
            text: translationManager.dummyBool
                  && qsTranslate("qml", "Applications")
        }
        TabButton {
            icon.source: "qrc:/images/application_error.png"
            text: translationManager.dummyBool
                  && qsTranslate("qml", "Blocked")
        }
        TabButton {
            icon.source: "qrc:/images/chart_line.png"
            text: translationManager.dummyBool
                  && qsTranslate("qml", "Statistics")
        }
        TabButton {
            icon.source: "qrc:/images/clock.png"
            text: translationManager.dummyBool
                  && qsTranslate("qml", "Schedule")
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        OptionsPage {}
        AddressesPage {}
        ApplicationsPage {}
        BlockedPage {}
        StatisticsPage {}
        SchedulePage {}
    }

    footer: Pane {
        RowLayout {
            anchors.right: parent.right

            Button {
                enabled: confFlagsEdited || confEdited || scheduleEdited
                icon.source: "qrc:/images/tick.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "OK")
                onClicked: {
                    if (confFlagsEdited || confEdited) {
                        if (!fortManager.saveConf(confFlagsEdited))
                            return;
                    }

                    mainPage.saved();
                    closeWindow();
                }
            }
            Button {
                enabled: confFlagsEdited || confEdited || scheduleEdited
                icon.source: "qrc:/images/accept.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Apply")
                onClicked: {
                    if (confFlagsEdited || confEdited) {
                        if (!fortManager.applyConf(confFlagsEdited))
                            return;
                    }

                    mainPage.saved();
                    resetEdited();
                }
            }
            Button {
                icon.source: "qrc:/images/cancel.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Cancel")
                onClicked: closeWindow()
            }
            Button {
                icon.source: "qrc:/images/cross.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Quit")
                onClicked: fortManager.exit()
            }
        }
    }
}
