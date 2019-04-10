import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../controls"
import com.fortfirewall 1.0

Page {
    id: mainPage

    signal opened()
    signal closed()
    signal editResetted()
    signal aboutToSave()
    signal saved()

    property bool confFlagsEdited
    property bool confEdited
    property bool othersEdited

    function setConfFlagsEdited() {
        confFlagsEdited = true;
    }

    function setConfEdited() {
        confEdited = true;
    }

    function setOthersEdited() {
        othersEdited = true;
    }

    function resetEdited() {
        confFlagsEdited = false;
        confEdited = false;
        othersEdited = false;

        editResetted();
    }

    function save(closeOnSuccess) {
        fortSettings.bulkUpdateBegin();

        mainPage.aboutToSave();

        var confSaved = true;
        if (confFlagsEdited || confEdited) {
            const confFlagsOnly = confFlagsEdited && !confEdited;
            confSaved = closeOnSuccess
                    ? fortManager.saveConf(confFlagsOnly)
                    : fortManager.applyConf(confFlagsOnly);
        }

        if (confSaved && othersEdited) {
            mainPage.saved();
        }

        fortSettings.bulkUpdateEnd();

        if (confSaved) {
            if (closeOnSuccess) {
                closeWindow();
            } else {
                resetEdited();
            }
        }
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
            width: parent.width

            RowLayout {
                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Logs")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.logsPath
                }

                VSeparator {}

                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Profile")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.profilePath
                }

                VSeparator {}

                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Statistics")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.statPath
                }

                VSeparator {}

                LinkButton {
                    text: translationManager.trTrigger
                          && qsTranslate("qml", "Releases")
                    tipText: link
                    onClicked: Qt.openUrlExternally(link)
                    readonly property string link: fortSettings.appUpdatesUrl
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                enabled: confFlagsEdited || confEdited || othersEdited
                icon.source: "qrc:/images/tick.png"
                text: translationManager.trTrigger
                      && qsTranslate("qml", "OK")
                onClicked: mainPage.save(true)
            }
            Button {
                enabled: confFlagsEdited || confEdited || othersEdited
                icon.source: "qrc:/images/accept.png"
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Apply")
                onClicked: mainPage.save(false)
            }
            Button {
                icon.source: "qrc:/images/cancel.png"
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Cancel")
                onClicked: closeWindow()
            }
        }
    }
}
