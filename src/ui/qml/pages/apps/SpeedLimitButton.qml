import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: (appGroup.speedLimitIn || appGroup.speedLimitOut)
                 ? "qrc:/images/flag_yellow.png"
                 : "qrc:/images/flag_green.png"
    text: translationManager.dummyBool
          && qsTranslate("qml", "Speed Limit")

    readonly property var speedLimitValues: [
        1024, 0, 100, 1024, 3 * 1024
    ]

    readonly property var speedLimitNames:
        translationManager.dummyBool
        && [
            qsTranslate("qml", "Custom"),
            qsTranslate("qml", "Never"),
            "100 KiB/s",
            "1 MiB/s",
            "3 MiB/s"
        ]

    ColumnLayout {
        SpinComboRow {
            names: speedLimitNames
            values: speedLimitValues
            label.text: translationManager.dummyBool
                        && qsTranslate("qml", "Download speed limit:")
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
                        && qsTranslate("qml", "Upload speed limit:")
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
