import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../controls"
import com.fortfirewall 1.0

BasePage {

    readonly property DriverManager driverManager: fortManager.driverManager

    property bool logReadingEnabled: false
    property bool addressResolvingEnabled: false

    // TODO: Use SHGetFileInfo() to get app's display name and icon
    property var appNames: []
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
        if (logReadingEnabled === enable)
            return;

        logReadingEnabled = enable;

        fortManager.setAppLogBlocked(enable);

        if (enable) {
            readLogAsync();
        } else {
            cancelDeviceIo();
        }
    }

    function switchResolveAddresses(enable) {
        if (addressResolvingEnabled === enable)
            return;

        addressResolvingEnabled = enable;

        if (!enable) {
            hostInfo.clear();
        }
    }

    function clearAppPaths() {
        appNames = [];
        appPaths = [];
        appPathIpMap = ({});
        appPathIpArray = ({});

        hostNames = ({});
        hostInfo.clear();

        refreshListViews();
    }

    function processLogBuffer() {
        var isNewEntry = false;

        while (logBuffer.read(logEntry)) {
            var path = getEntryPath(logEntry);
            var ipText = netUtil.ip4ToText(logEntry.ip);

            var ipTextsMap = appPathIpMap[path];
            if (!ipTextsMap) {
                ipTextsMap = ({});
                appPathIpMap[path] = ipTextsMap;
                appPathIpArray[path] = [];
                appNames.push(fileUtil.fileName(path));
                appPaths.push(path);

                isNewEntry = true;
            }

            var ipCount = ipTextsMap[ipText];
            ipTextsMap[ipText] = (ipCount || 0) + 1;

            var ipTextsArray = appPathIpArray[path];
            if (!ipCount) {
                if (ipTextsArray.length > 64) {
                    var oldIp = ipTextsArray.pop();
                    delete ipTextsMap[oldIp];
                }
                ipTextsArray.unshift(ipText);

                isNewEntry = true;
            }

            // Host name
            if (hostNames[ipText] === undefined) {
                hostNames[ipText] = false;
                if (addressResolvingEnabled) {
                    hostInfo.lookupHost(ipText);
                }
            }
        }

        if (isNewEntry) {
            refreshListViews();
        }
    }

    function getEntryPath(logEntry) {
        const kernelPath = logEntry.kernelPath;
        if (kernelPath) {
            return fileUtil.kernelPathToPath(kernelPath);
        }

        return osUtil.pidToPath(logEntry.pid);
    }

    function refreshListViews() {
        const curIndex = appListView.currentIndex;
        appListView.model = undefined;

        appListView.model = appNames;
        appListView.currentIndex = curIndex;
    }

    Connections {
        target: mainPage
        onClosed: {
            switchResolveAddresses(false);
            switchLogReading(false);
        }
    }

    Connections {
        target: driverManager
        onReadLogResult: {
            if (success) {
                processLogBuffer();
            }

            if (logReadingEnabled) {
                readLogAsync();
            }
        }
    }

    HostInfo {
        id: hostInfo
        onLookupFinished: {
            if (addressResolvingEnabled) {
                hostNames[address] = hostName;
                if (hostName) {
                    refreshListViews();
                }
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

                    highlightRangeMode: ListView.ApplyRange
                    highlightResizeDuration: 0
                    highlightMoveDuration: 200

                    highlight: Item {
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -7
                            radius: 2
                            border.width: 3
                            border.color: "blue"
                            color: "transparent"
                        }
                    }

                    delegate: Row {
                        width: appListView.width
                        spacing: 6

                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: 1
                            source: "qrc:/images/application.png"
                        }
                        Label {
                            font.pixelSize: 20
                            elide: Text.ElideRight
                            text: modelData
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

        TextFieldFrame {
            Layout.fillWidth: true
            text: appPaths[appListView.currentIndex] || ""
            onReleased: selectAll()
        }
    }
}
