import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../controls"
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

    readonly property var taskIntervalNames: [
        qsTranslate("qml", "Custom"),
        qsTranslate("qml", "Hourly"),
        qsTranslate("qml", "Each 6 hours"),
        qsTranslate("qml", "Each 12 hours"),
        qsTranslate("qml", "Daily"),
        qsTranslate("qml", "Weekly"),
        qsTranslate("qml", "Monthly")
    ]

    property bool scheduleEdited

    function setScheduleEdited() {
        scheduleEdited = true;

        setOthersEdited();
    }

    function onEditResetted() {  // override
        scheduleEdited = false;
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
                    text: qsTranslate("qml", "Name")
                }
                Label {
                    width: taskCellWidths[2]
                    text: qsTranslate("qml", "Interval, hours")
                }
                Label {
                    width: taskCellWidths[3]
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTranslate("qml", "Last Run")
                }
                Label {
                    width: taskCellWidths[4]
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTranslate("qml", "Last Success")
                }
                Item {
                    width: taskCellWidths[5]
                    height: parent.height
                }
            }

            HSeparator {}

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10
                clip: true

                model: taskManager.taskInfos

                delegate: TaskRow {
                    taskInfo: modelData
                }

                ScrollBar.vertical: ScrollBarControl {}
            }
        }
    }
}
