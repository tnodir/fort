import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../controls"
import com.fortfirewall 1.0

Page {
    id: mainPage

    signal opened()
    signal closed()
    signal editResetted()
    signal aboutToSave()
    signal saved()

    readonly property TaskInfoUpdateChecker updateChecker: taskManager.taskInfoUpdateChecker

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

    header: TabBarControl {
        id: tabBar
        currentIndex: swipeView.currentIndex

        TabButton {
            icon.source: "qrc:/images/cog.png"
            text: qsTranslate("qml", "Options")
        }
        TabButton {
            icon.source: "qrc:/images/link.png"
            text: qsTranslate("qml", "IPv4 Addresses")
        }
        TabButton {
            icon.source: "qrc:/images/application_double.png"
            text: qsTranslate("qml", "Applications")
        }
        TabButton {
            icon.source: "qrc:/images/application_error.png"
            text: qsTranslate("qml", "Blocked")
        }
        TabButton {
            icon.source: "qrc:/images/chart_line.png"
            text: qsTranslate("qml", "Statistics")
        }
        TabButton {
            icon.source: "qrc:/images/clock.png"
            text: qsTranslate("qml", "Schedule")
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
                    text: qsTranslate("qml", "Logs")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.logsPath
                }

                VSeparator {}

                LinkButton {
                    text: qsTranslate("qml", "Profile")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.profilePath
                }

                VSeparator {}

                LinkButton {
                    text: qsTranslate("qml", "Statistics")
                    tipText: path
                    onClicked: Qt.openUrlExternally("file:///" + path)
                    readonly property string path: fortSettings.statPath
                }

                VSeparator {}

                LinkButton {
                    text: qsTranslate("qml", "Releases")
                    tipText: link
                    onClicked: Qt.openUrlExternally(link)
                    readonly property string link: fortSettings.appUpdatesUrl
                }

                VSeparator {
                    visible: !!updateChecker.version
                }

                LinkButton {
                    visible: !!updateChecker.version
                    normalColor: "red"
                    text: qsTranslate("qml", "New version:")
                          + " v" + updateChecker.version
                    onClicked: Qt.openUrlExternally(updateChecker.downloadUrl)

                    PopupBox {
                        y: parent.y - height
                        visible: parent.hovered
                        Label {
                            textFormat: Text.MarkdownText
                            text: updateChecker.releaseText
                            onLinkActivated: Qt.openUrlExternally(link)
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                enabled: confFlagsEdited || confEdited || othersEdited
                icon.source: "qrc:/images/tick.png"
                text: qsTranslate("qml", "OK")
                onClicked: mainPage.save(true)
            }
            Button {
                enabled: confFlagsEdited || confEdited || othersEdited
                icon.source: "qrc:/images/accept.png"
                text: qsTranslate("qml", "Apply")
                onClicked: mainPage.save(false)
            }
            Button {
                icon.source: "qrc:/images/cancel.png"
                text: qsTranslate("qml", "Cancel")
                onClicked: closeWindow()
            }
        }
    }
}
