import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    readonly property DriverManager driverManager: fortManager.driverManager

    property bool enableLogReading: false

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
        appPaths = ([]);
        appPathsMap = ({});
    }

    function processLogBuffer() {
        while (logBuffer.read(logEntry)) {
            var path = logEntry.path;
            var ipText = logEntry.ipText;
console.log(">", path, ipText);

            var ipTextsMap = appPathsMap[path];
            if (!ipTextsMap) {
                ipTextsMap = ({});
                appPathsMap[path] = ipTextsMap;
            }

            ipTextsMap[ipText] = (ipTextsMap[ipText] || 0) + 1;
        }
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

        RowLayout {
            Button {
                text: QT_TRANSLATE_NOOP("qml", "Clear")
                onClicked: clearAppPaths()
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                text: QT_TRANSLATE_NOOP("qml", "Show Blocked Applications and Addresses")
                onToggled: switchLogReading(checked)
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
