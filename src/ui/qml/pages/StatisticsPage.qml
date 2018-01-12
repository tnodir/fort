import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"
import "log"
import com.fortfirewall 1.0

BasePage {
    id: statisticsPage

    readonly property LogManager logManager: fortManager.logManager
    readonly property AppStatModel appStatModel: logManager.appStatModel
    readonly property TrafListModel trafListModel:
        appStatModel.trafListModel(tabBar.currentIndex,
                                   appListView.currentIndex,
                                   currentAppPath)

    readonly property string currentAppPath: appListView.currentItemText

    readonly property var trafCellWidths: [
        trafsContainer.width * 0.34,
        trafsContainer.width * 0.22,
        trafsContainer.width * 0.22,
        trafsContainer.width * 0.22
    ]

    readonly property var trafUnitNames:
        translationManager.dummyBool
        && [
            qsTranslate("qml", "Adaptive"),
            qsTranslate("qml", "Bytes"),
            qsTranslate("qml", "KiB"),
            qsTranslate("qml", "MiB"),
            qsTranslate("qml", "GiB"),
            qsTranslate("qml", "TiB")
        ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            RoundButtonTip {
                enabled: appListView.count
                icon.source: "qrc:/images/arrow_refresh.png"
                tipText: translationManager.dummyBool
                         && qsTranslate("qml", "Refresh")
                onClicked: trafListModel.refresh()
            }

            ButtonMenu {
                enabled: appListView.count
                icon.source: "qrc:/images/bin_empty.png"
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Clear…")

                MenuItem {
                    enabled: appListView.currentIndex > 0
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Remove Application")
                    onTriggered: appStatModel.remove(
                                     appListView.currentIndex)
                }
                MenuItem {
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Reset Total")
                    onTriggered: trafListModel.resetAppTotals()
                }
                MenuItem {
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Clear All")
                    onTriggered: {
                        appListView.currentIndex = 0;
                        appStatModel.clear();
                    }
                }
            }

            TrafOptionsButton {}

            Row {
                spacing: 5

                Item {
                    width: 1
                    height: 1
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Units:")
                }

                ComboBox {
                    id: comboTrafUnit

                    currentIndex: firewallConf.trafUnit
                    model: trafUnitNames

                    onModelChanged: {
                        currentIndex = firewallConf.trafUnit;
                    }
                    onActivated: {
                        firewallConf.trafUnit = index;

                        fortManager.applyConfImmediateFlags();

                        trafListModel.refresh();
                    }
                }
            }

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
                    emptyIcon: "qrc:/images/application_double.png"
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

                    HSeparator {}

                    ListView {
                        id: trafListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 5
                        clip: true

                        model: trafListModel

                        delegate: TrafRow {}

                        ScrollBar.vertical: ScrollBarControl {}
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
