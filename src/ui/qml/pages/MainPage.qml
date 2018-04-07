import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import "../controls"
import com.fortfirewall 1.0

Page {
    id: mainPage

    signal opened()
    signal closed()
    signal aboutToSave()
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
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Options")
        }
        TabButton {
            icon.source: "qrc:/images/link.png"
            text: translationManager.trTrigger
                  && qsTranslate("qml", "IPv4 Addresses")
        }
        TabButton {
            icon.source: "qrc:/images/application_double.png"
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Applications")
        }
        TabButton {
            icon.source: "qrc:/images/application_error.png"
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Blocked")
        }
        TabButton {
            icon.source: "qrc:/images/chart_line.png"
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Statistics")
        }
        TabButton {
            icon.source: "qrc:/images/clock.png"
            text: translationManager.trTrigger
                  && qsTranslate("qml", "Schedule")
        }
    }

    SwipeViewControl {
        id: swipeView
        anchors.fill: parent

        interactive: false
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
                text: translationManager.trTrigger
                      && qsTranslate("qml", "OK")
                onClicked: {
                    mainPage.aboutToSave();

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
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Apply")
                onClicked: {
                    mainPage.aboutToSave();

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
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Cancel")
                onClicked: closeWindow()
            }
            Button {
                icon.source: "qrc:/images/cross.png"
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Quit")
                onClicked: fortManager.exit()
            }
        }
    }
}
