import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: "qrc:/images/database_save.png"
    text: translationManager.trTrigger
          && qsTranslate("qml", "Options…")

    readonly property var trafKeepDayValues: [
        60, -1, 90, 180, 365, 365 * 3
    ]

    readonly property var trafKeepDayNames:
        translationManager.trTrigger
        && [
            qsTranslate("qml", "Custom"),
            qsTranslate("qml", "Forever"),
            qsTranslate("qml", "3 months"),
            qsTranslate("qml", "6 months"),
            qsTranslate("qml", "1 year"),
            qsTranslate("qml", "3 years")
        ]

    readonly property var trafKeepMonthValues: [
        2, -1, 3, 6, 12, 36
    ]

    readonly property var trafKeepMonthNames:
        translationManager.trTrigger
        && [
            qsTranslate("qml", "Custom"),
            qsTranslate("qml", "Forever"),
            qsTranslate("qml", "3 months"),
            qsTranslate("qml", "6 months"),
            qsTranslate("qml", "1 year"),
            qsTranslate("qml", "3 years")
        ]

    readonly property var quotaValues: [
        10, 0, 100, 500, 1024, 8 * 1024, 10 * 1024, 30 * 1024,
        50 * 1024, 100 * 1024
    ]

    readonly property var quotaNames: {
        var list = translationManager.trTrigger
                && [qsTranslate("qml", "Custom"),
                    qsTranslate("qml", "Disabled")];

        const n = quotaValues.length;
        for (var i = list.length; i < n; ++i) {
            list.push(formatQuota(quotaValues[i]));
        }
        return list;
    }

    function formatQuota(mbytes) {
        return netUtil.formatDataSize(mbytes * 1024 * 1024, 1);
    }

    ColumnLayout {
        SpinDoubleRow {
            checkBox {
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Active period, hours:")
                checked: firewallConf.activePeriodEnabled
                onToggled: {
                    firewallConf.activePeriodEnabled = checkBox.checked;

                    setConfFlagsEdited();
                }
            }
            field1 {
                from: 0
                to: 24
                defaultValue: firewallConf.activePeriodFrom
                onValueEdited: {
                    firewallConf.activePeriodFrom = field1.value;

                    setConfFlagsEdited();
                }
            }
            field2 {
                from: 0
                to: 24
                defaultValue: firewallConf.activePeriodTo
                onValueEdited: {
                    firewallConf.activePeriodTo = field2.value;

                    setConfFlagsEdited();
                }
            }
        }

        SpinComboRow {
            values: {
                var arr = [];
                for (var i = 1; i <= 31; ++i) {
                    arr.push(i);
                }
                return arr;
            }
            checkBox {
                indicator: null
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Month starts on:")
            }
            field {
                from: 1
                to: 31
                defaultValue: firewallConf.monthStart
                onValueEdited: {
                    firewallConf.monthStart = field.value;

                    setConfFlagsEdited();
                }
            }
        }

        HSeparator {}

        SpinComboRow {
            names: trafKeepDayNames
            values: trafKeepDayValues
            checkBox {
                indicator: null
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Keep days for 'Hourly':")
            }
            field {
                from: -1
                defaultValue: firewallConf.trafHourKeepDays
                onValueEdited: {
                    firewallConf.trafHourKeepDays = field.value;

                    setConfFlagsEdited();
                }
            }
        }

        SpinComboRow {
            names: trafKeepDayNames
            values: trafKeepDayValues
            checkBox {
                indicator: null
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Keep days for 'Daily':")
            }
            field {
                from: -1
                defaultValue: firewallConf.trafDayKeepDays
                onValueEdited: {
                    firewallConf.trafDayKeepDays = field.value;

                    setConfFlagsEdited();
                }
            }
        }

        SpinComboRow {
            names: trafKeepMonthNames
            values: trafKeepMonthValues
            checkBox {
                indicator: null
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Keep months for 'Monthly':")
            }
            field {
                from: -1
                defaultValue: firewallConf.trafMonthKeepMonths
                onValueEdited: {
                    firewallConf.trafMonthKeepMonths = field.value;

                    setConfFlagsEdited();
                }
            }
        }

        HSeparator {}

        SpinComboRow {
            names: quotaNames
            values: quotaValues
            checkBox {
                indicator: null
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Day's Quota, MiB:")
            }
            field {
                from: 0
                to: 999 * 1024
                defaultValue: firewallConf.quotaDayMb
                onValueEdited: {
                    firewallConf.quotaDayMb = field.value;

                    setConfFlagsEdited();
                }
            }
        }

        SpinComboRow {
            names: quotaNames
            values: quotaValues
            checkBox {
                indicator: null
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Month's Quota, MiB:")
            }
            field {
                from: 0
                to: 999 * 1024
                defaultValue: firewallConf.quotaMonthMb
                onValueEdited: {
                    firewallConf.quotaMonthMb = field.value;

                    setConfFlagsEdited();
                }
            }
        }
    }
}
