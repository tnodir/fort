import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: !(appGroup.speedLimitIn || appGroup.speedLimitOut)
                 ? "qrc:/images/flag_green.png"
                 : "qrc:/images/flag_yellow.png"
    text: (translationManager.dummyBool
          && qsTranslate("qml", "Speed Limit: "))
          + speedLimitsText

    readonly property var speedLimitValues: [
        1024, 0, 100, 1024, 3 * 1024
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

    readonly property string speedLimitsText: {
        const limitIn = appGroup.speedLimitIn;
        const limitOut = appGroup.speedLimitOut;

        if (!(limitIn || limitOut))
            return speedLimitNames[1];

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
            label.text: translationManager.dummyBool
                        && qsTranslate("qml", "Download speed limit, KiB/s:")
            field {
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
            label.text: translationManager.dummyBool
                        && qsTranslate("qml", "Upload speed limit, KiB/s:")
            field {
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
