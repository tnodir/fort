import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "schedule"
import com.fortfirewall 1.0

BasePage {
    id: schedulePage

    signal saveTaskInfo()

    readonly property TaskManager taskManager: fortManager.taskManager

    readonly property var taskCellWidths: [
        tasksContainer.width * 0.05,
        tasksContainer.width * 0.24,
        tasksContainer.width * 0.3,
        tasksContainer.width * 0.18,
        tasksContainer.width * 0.18,
        tasksContainer.width * 0.05
    ]

    readonly property var taskIntervalHours: [
        3, 1, 6, 12, 24, 24 * 7, 24 * 30
    ]

    readonly property var taskIntervalNames:
        translationManager.dummyBool
        && [
            qsTranslate("qml", "Custom"),
            qsTranslate("qml", "Hourly"),
            qsTranslate("qml", "Each 6 hours"),
            qsTranslate("qml", "Each 12 hours"),
            qsTranslate("qml", "Daily"),
            qsTranslate("qml", "Weekly"),
            qsTranslate("qml", "Monthly")
        ]

    function getIntervalIndexByValue(value) {
        for (var i = taskIntervalHours.length; --i >= 0; ) {
            if (value === taskIntervalHours[i]) {
                return i;
            }
        }
        return 0;
    }

    function onSaved() {  // override
        if (!scheduleEdited) return;

        saveTaskInfo();
        taskManager.saveSettings(fortSettings);
    }

    Frame {
        anchors.fill: parent

        ColumnLayout {
            id: tasksContainer
            anchors.fill: parent

            Row {
                Layout.fillWidth: true
                spacing: 0

                Item {
                    width: taskCellWidths[0]
                    height: parent.height
                }
                Label {
                    width: taskCellWidths[1]
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Name")
                }
                Label {
                    width: taskCellWidths[2]
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Interval, hours")
                }
                Label {
                    width: taskCellWidths[3]
                    horizontalAlignment: Text.AlignHCenter
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Last Run")
                }
                Label {
                    width: taskCellWidths[4]
                    horizontalAlignment: Text.AlignHCenter
                    text: translationManager.dummyBool
                          && qsTranslate("qml", "Last Success")
                }
                Item {
                    width: taskCellWidths[5]
                    height: parent.height
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                interactive: false

                Repeater {
                    model: taskManager.taskInfos

                    TaskRow {
                        taskInfo: modelData
                    }
                }
            }
        }
    }
}
