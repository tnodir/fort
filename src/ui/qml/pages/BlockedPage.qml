import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../controls"
import com.fortfirewall 1.0

BasePage {

    readonly property LogManager logManager: fortManager.logManager
    readonly property AppBlockedModel appBlockedModel: logManager.appBlockedModel

    property bool logReadingEnabled: false
    property bool addressResolvingEnabled: false

    readonly property string currentAppPath:
        (appListView.currentIndex >= 0 && appListView.currentItem)
        ? appListView.currentItem.appPath : ""

    function switchLogReading(enable) {
        if (logReadingEnabled === enable)
            return;

        logReadingEnabled = enable;

        fortManager.setLogBlocked(enable);
    }

    function switchResolveAddresses(enable) {
        if (addressResolvingEnabled === enable)
            return;

        addressResolvingEnabled = enable;
    }

    function clearAppPaths() {
        logManager.clearModels();
    }

    Connections {
        target: mainPage
        onClosed: {
            switchResolveAddresses(false);
            switchLogReading(false);
        }
    }

    HostInfoCache {
        id: hostInfoCache
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Button {
                enabled: appListView.count
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Clear")
                onClicked: clearAppPaths()
            }

            CheckBox {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Resolve Addresses")
                onToggled: switchResolveAddresses(checked)
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                id: cbShowBlockedApps
                font.weight: Font.DemiBold
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Log Blocked Applications")
                onToggled: switchLogReading(checked)
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            RowLayout {
                anchors.fill: parent
                spacing: 20

                ListView {
                    id: appListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    model: appBlockedModel

                    highlightRangeMode: ListView.ApplyRange
                    highlightResizeDuration: 0
                    highlightMoveDuration: 200

                    highlight: Item {
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -7
                            radius: 2
                            border.width: 3
                            border.color: palette.highlight
                            color: "transparent"
                        }
                    }

                    delegate: Row {
                        id: appItem
                        width: appListView.width
                        spacing: 6

                        readonly property string appPath: modelData

                        // TODO: Use SHGetFileInfo() to get app's display name and icon
                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: 1
                            source: "qrc:/images/application.png"
                        }
                        Label {
                            font.pixelSize: 20
                            elide: Text.ElideRight
                            text: fileUtil.fileName(appItem.appPath)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            const index = appListView.indexAt(mouse.x, mouse.y);
                            if (index >= 0) {
                                appListView.currentIndex = index;
                            }
                        }
                    }
                }

                ListView {
                    id: ipListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 4

                    model: appBlockedModel.ipListModel(currentAppPath)

                    delegate: Label {
                        width: ipListView.width
                        elide: Text.ElideRight
                        text: (addressResolvingEnabled && hostInfoCache.dummyBool
                               && hostInfoCache.hostName(ipText)) || ipText

                        readonly property string ipText: modelData
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
