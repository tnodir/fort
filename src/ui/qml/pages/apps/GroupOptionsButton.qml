import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: "qrc:/images/application_key.png"
    text: qsTranslate("qml", "Options…")

    readonly property var speedLimitValues: [
        10, 0, 20, 30, 50, 75, 100, 150, 200, 300, 500, 900,
        1024, 1.5 * 1024, 2 * 1024, 3 * 1024, 5 * 1024, 7.5 * 1024,
        10 * 1024, 15 * 1024, 20 * 1024, 30 * 1024, 50 * 1024
    ]

    readonly property var speedLimitNames: {
        var list = [qsTranslate("qml", "Custom"),
                    qsTranslate("qml", "Disabled")];

        const n = speedLimitValues.length;
        for (var i = list.length; i < n; ++i) {
            list.push(formatSpeed(speedLimitValues[i]));
        }
        return list;
    }

    function formatSpeed(kbytes) {
        return netUtil.formatSpeed(kbytes * 1024);
    }

    ColumnLayout {
        SpinComboRow {
            names: speedLimitNames
            values: speedLimitValues
            checkBox {
                text: qsTranslate("qml", "Download speed limit, KiB/s:")
                checked: appGroup.limitInEnabled
                onToggled: {
                    appGroup.limitInEnabled = checkBox.checked;

                    setConfEdited();
                }
            }
            field {
                from: 0
                to: 99999
                defaultValue: appGroup.speedLimitIn
                onValueEdited: {
                    appGroup.speedLimitIn = field.value;

                    setConfEdited();
                }
            }
        }

        SpinComboRow {
            names: speedLimitNames
            values: speedLimitValues
            checkBox {
                text: qsTranslate("qml", "Upload speed limit, KiB/s:")
                checked: appGroup.limitOutEnabled
                onToggled: {
                    appGroup.limitOutEnabled = checkBox.checked;

                    setConfEdited();
                }
            }
            field {
                from: 0
                to: 99999
                defaultValue: appGroup.speedLimitOut
                onValueEdited: {
                    appGroup.speedLimitOut = field.value;

                    setConfEdited();
                }
            }
        }

        HSeparator {}

        CheckBox {
            text: qsTranslate("qml", "Fragment first TCP packet")
            checked: appGroup.fragmentPacket
            onToggled: {
                appGroup.fragmentPacket = checked;

                setConfEdited();
            }
        }
    }
}
