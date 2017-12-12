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
    readonly property TrafListModel trafListModel:
        appStatModel.trafListModel(tabBar.currentIndex, appListView.currentIndex)

    readonly property string currentAppPath:
        (appListView.currentIndex >= 0 && appListView.currentItem)
        ? appListView.currentItem.appPath : ""

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
            spacing: 15

            Button {
                enabled: appListView.count
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Refresh")
                onClicked: trafListModel.refresh()
            }

            Row {
                spacing: 5

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Units:")
                }

                ComboBox {
                    id: comboTrafUnit

                    currentIndex: firewallConf.trafUnit
                    model: trafUnitNames

                    onActivated: {
                        firewallConf.trafUnit = index;

                        fortManager.applyConfImmediateValues();

                        trafListModel.refresh();
                    }
                }
            }

            Button {
                enabled: appListView.count
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Clear")
                onClicked: {
                    appStatModel.clear();

                    appListView.currentIndex = 0;
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

                    fortManager.applyConfImmediateValues();
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

                        model: trafListModel

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
