import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../controls"
import com.fortfirewall 1.0

Row {

    property TaskInfo taskInfo

    function formatDate(date) {
        return Qt.formatDateTime(date, "yyyy-MM-dd hh:mm:ss");
    }

    function saveTaskInfo() {
        taskInfo.enabled = cbEnabled.checked;
        taskInfo.intervalHours = spinCombo.field.value;
    }

    Connections {
        target: schedulePage
        onSaveTaskInfo: saveTaskInfo()
    }

    CheckBox {
        id: cbEnabled
        width: taskCellWidths[0]
        checked: taskInfo.enabled
        onToggled: setScheduleEdited()
    }

    Label {
        width: taskCellWidths[1]
        height: parent.height
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        fontSizeMode: Text.Fit
        text: translationManager.dummyBool
              && taskInfo.title
    }

    SpinCombo {
        id: spinCombo
        width: taskCellWidths[2]

        fieldPreferredWidth: 110

        names: taskIntervalNames
        values: taskIntervalHours

        field {
            from: 1
            to: 24 * 30 * 12  // ~Year
            value: taskInfo.intervalHours

            onValueChanged: {
                const value = field.value;
                if (value != taskInfo.intervalHours) {
                    setScheduleEdited();
                }
            }
        }
    }

    Label {
        width: taskCellWidths[3]
        height: parent.height
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        fontSizeMode: Text.Fit
        text: formatDate(taskInfo.lastRun)
    }

    Label {
        width: taskCellWidths[4]
        height: parent.height
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        fontSizeMode: Text.Fit
        text: formatDate(taskInfo.lastSuccess)
    }

    Item {
        width: taskCellWidths[5]
        height: parent.height

        RoundButton {
            id: btRun
            anchors.centerIn: parent
            width: 30
            height: width
            icon.source: taskInfo.running ? "qrc:/images/cancel.png"
                                          : "qrc:/images/run.png"
            onClicked: taskInfo.running ? taskInfo.abort()
                                        : taskInfo.run()
        }

        RotationAnimator {
            target: btRun
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: 4000
            easing.type: Easing.OutBack
            running: taskInfo.running
            onStopped: {
                btRun.rotation = 0;
            }
        }
    }
}
