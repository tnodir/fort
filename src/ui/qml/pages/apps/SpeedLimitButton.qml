import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: !((appGroup.limitInEnabled && appGroup.speedLimitIn)
                   || (appGroup.limitOutEnabled && appGroup.speedLimitOut))
                 ? "qrc:/images/flag_green.png"
                 : "qrc:/images/flag_yellow.png"
    text: (translationManager.trTrigger
          && qsTranslate("qml", "Speed Limit: "))
          + speedLimitsText

    readonly property var speedLimitValues: [
        10, 0, 20, 30, 50, 75, 100, 150, 200, 300, 500, 900,
        1024, 1.5 * 1024, 2 * 1024, 3 * 1024, 5 * 1024, 7.5 * 1024,
        10 * 1024, 15 * 1024, 20 * 1024, 30 * 1024, 50 * 1024
    ]

    readonly property var speedLimitNames: {
        var list = translationManager.trTrigger
                && [qsTranslate("qml", "Custom"),
                    qsTranslate("qml", "Disabled")];

        const n = speedLimitValues.length;
        for (var i = list.length; i < n; ++i) {
            list.push(formatSpeed(speedLimitValues[i]));
        }
        return list;
    }

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

    function formatSpeed(kbytes) {
        const prec = (kbytes < 1024) ? 0 : 1;
        return netUtil.formatDataSize(kbytes * 1024, prec) + "/s";
    }

    ColumnLayout {
        SpinComboRow {
            names: speedLimitNames
            values: speedLimitValues
            checkBox {
                text: translationManager.trTrigger
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
                to: 99999
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
                text: translationManager.trTrigger
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
                to: 99999
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
