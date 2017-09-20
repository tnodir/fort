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

    CheckBox {
        width: taskCellWidths[0]
        checked: taskInfo.enabled
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

            editable: true
            width: 180
            from: 0
            value: taskInfo.intervalHours
            to: 24 * 30 * 12  // ~Year

            onValueChanged: {
                comboInterval.currentIndex = getIntervalIndexByValue(value);
            }
        }

        ComboBox {
            id: comboInterval
            Layout.fillWidth: true

            model: taskIntervalNames

            onModelChanged: {
                fieldInterval.onValueChanged();
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
}
