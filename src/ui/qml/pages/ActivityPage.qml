import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    readonly property DriverManager driverManager: fortManager.driverManager

    property alias enableLogReading: cbShowBlockedApps.checked

    property var appPaths: []
    property var appPathIpMap: ({})
    property var appPathIpArray: ({})

    property var hostNames: ({})

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
        appPaths = [];
        appPathIpMap = ({});
        appPathIpArray = ({});

        hostNames = ({});
        hostInfo.abortHostLookups();

        refreshListViews();
    }

    function processLogBuffer() {
        while (logBuffer.read(logEntry)) {
            var path = getEntryPath(logEntry);
            var ipText = netUtil.ip4ToText(logEntry.ip);

            var ipTextsMap = appPathIpMap[path];
            if (!ipTextsMap) {
                ipTextsMap = ({});
                appPathIpMap[path] = ipTextsMap;
                appPathIpArray[path] = [];
                appPaths.push(path);
            }

            var ipCount = ipTextsMap[ipText];
            ipTextsMap[ipText] = (ipCount || 0) + 1;

            var ipTextsArray = appPathIpArray[path];
            if (!ipCount) {
                ipTextsArray.push(ipText);
            }

            // Host name
            if (hostNames[ipText] === undefined) {
                hostNames[ipText] = false;
                hostInfo.lookupHost(ipText);
            }
        }

        refreshListViews();
    }

    function getEntryPath(logEntry) {
        var dosPath = logEntry.dosPath;
        if (!dosPath) {
            dosPath = osUtil.pidToDosPath(logEntry.pid);
        }

        return fileUtil.dosPathToPath(dosPath);
    }

    function refreshListViews() {
        const curIndex = appListView.currentIndex;
        appListView.model = undefined;

        appListView.model = appPaths;
        appListView.currentIndex = curIndex;
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

    HostInfo {
        id: hostInfo
        onHostLookedup: {
            if (success) {
                hostNames[name] = hostName;
                refreshListViews();
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
                text: QT_TRANSLATE_NOOP("qml", "Copy Path")
                readonly property Item currentItem: appListView.currentItem
                onClicked: {
                    osUtil.setClipboardData(currentItem.text);
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                id: cbShowBlockedApps
                text: QT_TRANSLATE_NOOP("qml", "Log Blocked Applications")
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

                    highlightRangeMode: ListView.ApplyRange
                    highlightResizeDuration: 0
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
                        width: appListView.width
                        font.pixelSize: 20
                        elide: Text.ElideRight
                        text: modelData
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

                    model: {
                        const curIndex = appListView.currentIndex;
                        if (curIndex < 0)
                            return undefined;

                        const path = appPaths[curIndex];
                        return appPathIpArray[path];
                    }

                    delegate: Label {
                        width: ipListView.width
                        elide: Text.ElideRight
                        text: hostNames[ipText] || ipText
                        readonly property string ipText: modelData
                    }
                }
            }
        }
    }
}
