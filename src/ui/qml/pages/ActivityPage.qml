import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    readonly property DriverManager driverManager: fortManager.driverManager

    property alias enableLogReading: cbShowBlockedApps.checked

    property var appPaths: []
    property var appPathsMap: ({})

    function readLogAsync() {
        driverManager.readLogAsync(logBuffer);
    }

    function cancelDeviceIo() {
        driverManager.cancelDeviceIo();
    }

    function switchLogReading(enable) {
        enableLogReading = enable;

        fortManager.setAppLogBlocked(enable);

        if (enable) {
            readLogAsync();
        } else {
            cancelDeviceIo();
        }
    }

    function clearAppPaths() {
        listView.model = undefined;

        appPaths = ([]);
        appPathsMap = ({});
    }

    function processLogBuffer() {
        const curIndex = listView.currentIndex;
        listView.model = undefined;

        while (logBuffer.read(logEntry)) {
            var path = getEntryPath(logEntry);
            var ipText = netUtil.ip4ToText(logEntry.ip);

            var ipTextsMap = appPathsMap[path];
            if (!ipTextsMap) {
                ipTextsMap = ({});
                appPathsMap[path] = ipTextsMap;

                curIndex = appPaths.length;
                appPaths.push(path);
            }

            ipTextsMap[ipText] = (ipTextsMap[ipText] || 0) + 1;
        }

        listView.model = appPaths;
        listView.currentIndex = curIndex;
    }

    function getEntryPath(logEntry) {
        var dosPath = logEntry.dosPath;
        if (!dosPath) {
            dosPath = osUtil.pidToDosPath(logEntry.pid);
        }

        return fileUtil.dosPathToPath(dosPath);
    }

    Connections {
        target: mainPage
        onClosed: switchLogReading(false)
    }

    Connections {
        target: driverManager
        onReadLogResult: {
            if (success) {
                processLogBuffer();
            }

            if (enableLogReading) {
                readLogAsync();
            }
        }
    }

    LogBuffer {
        id: logBuffer
    }
    LogEntry {
        id: logEntry
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Button {
                enabled: btCopy.enabled
                text: QT_TRANSLATE_NOOP("qml", "Clear")
                onClicked: clearAppPaths()
            }
            Button {
                id: btCopy
                enabled: currentItem
                text: QT_TRANSLATE_NOOP("qml", "Copy")
                readonly property Item currentItem: listView.currentItem
                onClicked: {
                    osUtil.setClipboardData(currentItem.text);
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                id: cbShowBlockedApps
                text: QT_TRANSLATE_NOOP("qml", "Show Blocked Applications and Addresses")
                onToggled: switchLogReading(checked)
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: listView
                anchors.fill: parent
                spacing: 10

                highlightRangeMode: ListView.ApplyRange
                highlightResizeDuration: 150
                highlightMoveDuration: 200

                highlight: Item {
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -7
                        radius: 2
                        border.width: 3
                        border.color: "black"
                        color: "transparent"
                    }
                }

                delegate: Label {
                    width: listView.width
                    font.pixelSize: 20
                    elide: Text.ElideRight
                    text: modelData
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        const index = listView.indexAt(mouse.x, mouse.y);
                        if (index >= 0) {
                            listView.currentIndex = index;
                        }
                    }
                }
            }
        }
    }
}
