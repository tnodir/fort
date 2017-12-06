import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../controls"
import "log"
import com.fortfirewall 1.0

BasePage {
    id: statisticsPage

    readonly property LogManager logManager: fortManager.logManager
    readonly property AppStatModel appStatModel: logManager.appStatModel

    readonly property string currentAppPath:
        (appListView.currentIndex >= 0 && appListView.currentItem)
        ? appListView.currentItem.appPath : ""

    readonly property var trafCellWidths: [
        trafsContainer.width * 0.4,
        trafsContainer.width * 0.2,
        trafsContainer.width * 0.2,
        trafsContainer.width * 0.2
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            Switch {
                id: cbShowBlockedApps
                font.weight: Font.DemiBold
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Collect Traffic Statistics")
                checked: firewallConf.logStat
                onToggled: {
                    if (firewallConf.logStat === checked)
                        return;

                    firewallConf.logStat = checked;

                    fortManager.applyConfImmediateFlags();
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            RowLayout {
                anchors.fill: parent
                spacing: 20

                AppListView {
                    id: appListView
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    Layout.fillHeight: true

                    model: appStatModel

                    emptyText: translationManager.dummyBool
                               && qsTranslate("qml", "All")
                }

                ColumnLayout {
                    id: trafsContainer
                    Layout.fillWidth: true
                    Layout.preferredWidth: 200
                    Layout.fillHeight: true
                    spacing: 10

                    TabBar {
                        id: tabBar
                        Layout.fillWidth: true

                        TabButton {
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Hourly", "Stat")
                        }
                        TabButton {
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Daily", "Stat")
                        }
                        TabButton {
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Monthly", "Stat")
                        }
                        TabButton {
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Total", "Stat")
                        }
                    }

                    Row {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            width: trafCellWidths[0]
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Date")
                        }
                        Label {
                            width: trafCellWidths[1]
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Download")
                        }
                        Label {
                            width: trafCellWidths[2]
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Upload")
                        }
                        Label {
                            width: trafCellWidths[3]
                            text: translationManager.dummyBool
                                  && qsTranslate("qml", "Sum")
                        }
                    }

                    Frame {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                    }

                    ListView {
                        id: trafListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 4
                        clip: true

                        model: appStatModel.trafListModel(tabBar.currentIndex,
                                                          currentAppPath)

                        delegate: TrafRow {}
                    }
                }
            }
        }

        TextFieldFrame {
            Layout.fillWidth: true
            text: currentAppPath || ""
        }
    }
}
