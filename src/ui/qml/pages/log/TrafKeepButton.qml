import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: "qrc:/images/database_save.png"
    text: translationManager.dummyBool
          && qsTranslate("qml", "Keep")

    readonly property var trafKeepDayValues: [
        90, -1, 90, 180, 365, 365 * 3
    ]

    readonly property var trafKeepDayNames:
        translationManager.dummyBool
        && [
            qsTranslate("qml", "Custom"),
            qsTranslate("qml", "Forever"),
            qsTranslate("qml", "3 months"),
            qsTranslate("qml", "6 months"),
            qsTranslate("qml", "1 year"),
            qsTranslate("qml", "3 years")
        ]

    readonly property var trafKeepMonthValues: [
        360, -1, 3, 6, 12, 360
    ]

    readonly property var trafKeepMonthNames:
        translationManager.dummyBool
        && [
            qsTranslate("qml", "Custom"),
            qsTranslate("qml", "Forever"),
            qsTranslate("qml", "3 months"),
            qsTranslate("qml", "6 months"),
            qsTranslate("qml", "1 year"),
            qsTranslate("qml", "3 years")
        ]

    ColumnLayout {
        TrafKeepRow {
            names: trafKeepDayNames
            values: trafKeepDayValues
            label.text: translationManager.dummyBool
                        && qsTranslate("qml", "Days for 'Hourly':")
            field {
                value: firewallConf.trafHourKeepDays
                onValueChanged: {
                    const value = field.value;
                    if (firewallConf.trafHourKeepDays == value)
                        return;

                    firewallConf.trafHourKeepDays = value;

                    fortManager.applyConfImmediateKeys();
                }
            }
        }

        TrafKeepRow {
            names: trafKeepDayNames
            values: trafKeepDayValues
            label.text: translationManager.dummyBool
                        && qsTranslate("qml", "Days for 'Daily':")
            field {
                value: firewallConf.trafDayKeepDays
                onValueChanged: {
                    const value = field.value;
                    if (firewallConf.trafDayKeepDays == value)
                        return;

                    firewallConf.trafDayKeepDays = value;

                    fortManager.applyConfImmediateKeys();
                }
            }
        }

        TrafKeepRow {
            names: trafKeepMonthNames
            values: trafKeepMonthValues
            label.text: translationManager.dummyBool
                        && qsTranslate("qml", "Months for 'Monthly':")
            field {
                value: firewallConf.trafMonthKeepMonths
                onValueChanged: {
                    const value = field.value;
                    if (firewallConf.trafMonthKeepMonths == value)
                        return;

                    firewallConf.trafMonthKeepMonths = value;

                    fortManager.applyConfImmediateKeys();
                }
            }
        }
    }
}
