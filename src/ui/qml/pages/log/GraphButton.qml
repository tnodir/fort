import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../../controls"
import com.fortfirewall 1.0

ButtonPopup {

    icon.source: "qrc:/images/chart_bar.png"
    text: translationManager.trTrigger
          && qsTranslate("qml", "Graph…")

    function save() {
        fortSettings.graphWindowAlwaysOnTop = cbAlwaysOnTop.checked;
        fortSettings.graphWindowFrameless = cbFrameless.checked;
        fortSettings.graphWindowClickThrough = cbClickThrough.checked;
        fortSettings.graphWindowHideOnHover = cbHideOnHover.checked;

        fortSettings.graphWindowOpacity = rowOpacity.field.value;
        fortSettings.graphWindowHoverOpacity = rowHoverOpacity.field.value;
        fortSettings.graphWindowMaxSeconds = rowMaxSeconds.field.value;

        fortSettings.graphWindowColor = rowColor.selectedColor;
        fortSettings.graphWindowColorIn = rowColorIn.selectedColor;
        fortSettings.graphWindowColorOut = rowColorOut.selectedColor;
        fortSettings.graphWindowAxisColor = rowAxisColor.selectedColor;
        fortSettings.graphWindowTickLabelColor = rowTickLabelColor.selectedColor;
        fortSettings.graphWindowLabelColor = rowLabelColor.selectedColor;
        fortSettings.graphWindowGridColor = rowGridColor.selectedColor;
    }

    RowLayout {
        spacing: 10

        ColumnLayout {
            CheckBox {
                id: cbAlwaysOnTop
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Always on top")
                checked: fortSettings.graphWindowAlwaysOnTop
                onToggled: setGraphEdited()
            }

            CheckBox {
                id: cbFrameless
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Frameless")
                checked: fortSettings.graphWindowFrameless
                onToggled: setGraphEdited()
            }

            CheckBox {
                id: cbClickThrough
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Click through")
                checked: fortSettings.graphWindowClickThrough
                onToggled: setGraphEdited()
            }

            CheckBox {
                id: cbHideOnHover
                text: translationManager.trTrigger
                      && qsTranslate("qml", "Hide on hover")
                checked: fortSettings.graphWindowHideOnHover
                onToggled: setGraphEdited()
            }

            HSeparator {}

            LabelSpinRow {
                id: rowOpacity
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Opacity, %")) + ":"
                field {
                    from: 0
                    to: 100
                    defaultValue: fortSettings.graphWindowOpacity
                    onValueEdited: setGraphEdited()
                }
            }

            LabelSpinRow {
                id: rowHoverOpacity
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Hover opacity, %")) + ":"
                field {
                    from: 0
                    to: 100
                    defaultValue: fortSettings.graphWindowHoverOpacity
                    onValueEdited: setGraphEdited()
                }
            }

            LabelSpinRow {
                id: rowMaxSeconds
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Max seconds")) + ":"
                field {
                    from: 0
                    to: 9999
                    defaultValue: fortSettings.graphWindowMaxSeconds
                    onValueEdited: setGraphEdited()
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }

        VSeparator {}

        ColumnLayout {
            LabelColorRow {
                id: rowColor
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Background")) + ":"
                defaultColor: fortSettings.graphWindowColor
                onColorEdited: setGraphEdited()
            }

            LabelColorRow {
                id: rowColorIn
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Download")) + ":"
                defaultColor: fortSettings.graphWindowColorIn
                onColorEdited: setGraphEdited()
            }

            LabelColorRow {
                id: rowColorOut
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Upload")) + ":"
                defaultColor: fortSettings.graphWindowColorOut
                onColorEdited: setGraphEdited()
            }

            LabelColorRow {
                id: rowAxisColor
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Axis")) + ":"
                defaultColor: fortSettings.graphWindowAxisColor
                onColorEdited: setGraphEdited()
            }

            LabelColorRow {
                id: rowTickLabelColor
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Tick label")) + ":"
                defaultColor: fortSettings.graphWindowTickLabelColor
                onColorEdited: setGraphEdited()
            }

            LabelColorRow {
                id: rowLabelColor
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Label")) + ":"
                defaultColor: fortSettings.graphWindowLabelColor
                onColorEdited: setGraphEdited()
            }

            LabelColorRow {
                id: rowGridColor
                label.text: (translationManager.trTrigger
                             && qsTranslate("qml", "Grid")) + ":"
                defaultColor: fortSettings.graphWindowGridColor
                onColorEdited: setGraphEdited()
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
