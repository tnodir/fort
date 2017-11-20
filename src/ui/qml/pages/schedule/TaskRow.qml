import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

Row {

    property var cellWidths

    property TaskInfo taskInfo

    function formatDate(date) {
        return Qt.formatDateTime(date, "yyyy-MM-dd hh:mm:ss");
    }

    function saveTaskInfo() {
        taskInfo.enabled = cbEnabled.checked;
        taskInfo.intervalHours = fieldInterval.value;
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

    RowLayout {
        width: taskCellWidths[2]

        SpinBox {
            id: fieldInterval
            Layout.fillWidth: true
            Layout.preferredWidth: 110

            editable: true
            width: 180
            from: 1
            value: taskInfo.intervalHours
            to: 24 * 30 * 12  // ~Year

            onValueChanged: {
                comboInterval.updateIndex(value);

                if (value != taskInfo.intervalHours) {
                    setScheduleEdited();
                }
            }
        }

        ComboBox {
            id: comboInterval
            Layout.fillWidth: true

            model: taskIntervalNames

            function updateIndex(value) {
                currentIndex = getIntervalIndexByValue(value);
            }

            onModelChanged: {
                updateIndex(fieldInterval.value);
            }
            onActivated: {
                fieldInterval.value = taskIntervalHours[index];
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
