import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: !((appGroup.limitInEnabled && appGroup.speedLimitIn)
                   || (appGroup.limitOutEnabled && appGroup.speedLimitOut))
                 ? "qrc:/images/flag_green.png"
                 : "qrc:/images/flag_yellow.png"
    text: (translationManager.dummyBool
          && qsTranslate("qml", "Speed Limit: "))
          + speedLimitsText

    readonly property var speedLimitValues: [
        1, 0, 100, 1024, 3 * 1024
    ]

    readonly property var speedLimitNames:
        translationManager.dummyBool
        && [
            qsTranslate("qml", "Custom"),
            qsTranslate("qml", "Disabled"),
            formatSpeed(speedLimitValues[2]),
            formatSpeed(speedLimitValues[3]),
            formatSpeed(speedLimitValues[4])
        ]

    readonly property var speedLimitDisabledText: speedLimitNames[1]

    readonly property string speedLimitsText: {
        const limitIn = appGroup.limitInEnabled
                ? appGroup.speedLimitIn : 0;
        const limitOut = appGroup.limitOutEnabled
                ? appGroup.speedLimitOut : 0;

        if (!(limitIn || limitOut))
            return speedLimitDisabledText;

        var text = "";
        if (limitIn) {
            text = "DL " + formatSpeed(limitIn);
        }
        if (limitOut) {
            text += (text ? "; " : "")
                    + "UL " + formatSpeed(limitOut);
        }
        return text;
    }

    function formatSpeed(bytes) {
        return netUtil.formatDataSize(bytes * 1024, 0) + "/s";
    }

    ColumnLayout {
        SpinComboRow {
            names: speedLimitNames
            values: speedLimitValues
            checkBox {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Download speed limit, KiB/s:")
                checked: appGroup.limitInEnabled
                onCheckedChanged: {
                    const value = checkBox.checked;
                    if (appGroup.limitInEnabled == value)
                        return;

                    appGroup.limitInEnabled = value;

                    setConfEdited();
                }
            }
            field {
                from: 0
                value: appGroup.speedLimitIn
                onValueChanged: {
                    const value = field.value;
                    if (appGroup.speedLimitIn == value)
                        return;

                    appGroup.speedLimitIn = value;

                    setConfEdited();
                }
            }
        }

        SpinComboRow {
            names: speedLimitNames
            values: speedLimitValues
            checkBox {
                text: translationManager.dummyBool
                      && qsTranslate("qml", "Upload speed limit, KiB/s:")
                checked: appGroup.limitOutEnabled
                onCheckedChanged: {
                    const value = checkBox.checked;
                    if (appGroup.limitOutEnabled == value)
                        return;

                    appGroup.limitOutEnabled = value;

                    setConfEdited();
                }
            }
            field {
                from: 0
                value: appGroup.speedLimitOut
                onValueChanged: {
                    const value = field.value;
                    if (appGroup.speedLimitOut == value)
                        return;

                    appGroup.speedLimitOut = value;

                    setConfEdited();
                }
            }
        }
    }
}
