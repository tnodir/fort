#ifndef GRAPHOPTIONSAREA_H
#define GRAPHOPTIONSAREA_H

#include <QWidget>

#include <array>
#include <functional>

#include <user/inigraph.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)

class FontSelector;
class LabelColor;
class LabelSpin;
class LabelSpinCombo;

class GraphOptionsArea : public QWidget
{
    Q_OBJECT

public:
    explicit GraphOptionsArea(const IniGraph &iniGraph, QWidget *parent = nullptr);

    const IniGraph &iniGraph() const { return m_iniGraph; }
    IniGraph &iniGraph() { return m_iniGraph; }

signals:
    void edited();

public slots:
    void resetToDefault();

    void retranslateUi();

private:
    // The callback of the graph's option's control: sets the changed option and emits edited()
    template<typename T>
    std::function<void(T)> iniGraphSetter(
            T (IniGraph::*iniValue)() const, void (IniGraph::*setIniValue)(T));

    // The graph's option's check box: its toggling sets the option and emits edited()
    QCheckBox *createIniGraphCheckBox(
            bool (IniGraph::*iniFlag)() const, void (IniGraph::*setIniFlag)(bool));

    // The graph's option's combo box: its activation sets the option and emits edited()
    QComboBox *createIniGraphComboBox(const QStringList &texts, int (IniGraph::*iniValue)() const,
            void (IniGraph::*setIniValue)(int));

    void retranslateFixedSpeedCombo();
    void retranslateGraphAxisLines();
    void retranslateGraphColors();
    void retranslateGraphTypeCombo();
    void retranslateSpeedPositionCombo();

    void setupUi();
    QLayout *setupColumns();
    QLayout *setupColumn1();
    QLayout *setupColumn2();
    QLayout *setupColumn3();

    void setupWindowBox();
    void setupWindowCheckboxes();
    void setupWindowOpacity();
    void setupWindowBorder();
    void setupGeometryBox();
    void saveGraphWindowGeometry();
    void setupGeometryControls();
    LabelSpin *createGeometrySpin(int value, int min);
    void onGeometryEdited();
    void setupTaskbarBox();
    void setupGraphBox();
    QLayout *setupGraphTypeLayout();
    QLayout *setupGraphShowLayout();
    void setupGraphShowCheckboxes();
    void setupGraphOptions();
    void setupGraphFixedSpeed();
    void setupGraphViewOptions();
    QLayout *setupGraphAxisLinesLayout();
    QCheckBox *createGraphAxisLine(GraphAxisLine line);
    void resetGraphAxisLines();
    void setGraphAxisLine(GraphAxisLine line, bool on);
    FontSelector *createGraphFont(GraphFontType type);
    void resetGraphFont(FontSelector *c, GraphFontType type);
    void setGraphFont(GraphFontType type, const QFont &v);
    void setupSpeedBox();
    QLayout *setupSpeedPositionLayout();
    QLayout *setupTrafUnitsLayout();
    void setupColorsBox();
    void setupGraphColors();
    LabelColor *createGraphColor(GraphColorType type);
    void setGraphColor(GraphColorType type, bool dark, const QColor &v);

private:
    IniGraph m_iniGraph;

    QGroupBox *m_gbWindow = nullptr;
    QGroupBox *m_gbGeometry = nullptr;
    QGroupBox *m_gbTaskbar = nullptr;
    QGroupBox *m_gbGraph = nullptr;
    QGroupBox *m_gbSpeed = nullptr;
    QGroupBox *m_gbColors = nullptr;

    QCheckBox *m_cbGraphHideOnClose = nullptr;
    QCheckBox *m_cbGraphAlwaysOnTop = nullptr;
    QCheckBox *m_cbGraphFrameless = nullptr;
    QCheckBox *m_cbGraphClickThrough = nullptr;
    QCheckBox *m_cbGraphHideOnHover = nullptr;
    LabelSpin *m_graphOpacity = nullptr;
    LabelSpin *m_graphHoverOpacity = nullptr;
    LabelSpin *m_graphBorderWidth = nullptr;

    QCheckBox *m_cbGraphMaximized = nullptr;
    LabelSpin *m_graphX = nullptr;
    LabelSpin *m_graphY = nullptr;
    LabelSpin *m_graphWidth = nullptr;
    LabelSpin *m_graphHeight = nullptr;

    QCheckBox *m_cbGraphShowInTaskbar = nullptr;
    LabelSpin *m_graphTaskbarWidth = nullptr;

    QCheckBox *m_cbGraphShowGraph = nullptr;
    QLabel *m_labelGraphType = nullptr;
    QComboBox *m_comboGraphType = nullptr;
    QLabel *m_labelGraphShow = nullptr;
    QCheckBox *m_cbGraphShowTotal = nullptr;
    QCheckBox *m_cbGraphShowIn = nullptr;
    QCheckBox *m_cbGraphShowOut = nullptr;
    LabelSpin *m_graphBarWidth = nullptr;
    LabelSpin *m_graphMaxSeconds = nullptr;
    LabelSpinCombo *m_graphFixedSpeed = nullptr;
    QCheckBox *m_cbGraphAnimation = nullptr;
    QLabel *m_labelGraphAxisLines = nullptr;
    std::array<QCheckBox *, GraphAxisLineCount> m_cbGraphAxisLines = {};
    QCheckBox *m_cbGraphAxisTicks = nullptr;
    QCheckBox *m_cbGraphGridLines = nullptr;
    QCheckBox *m_cbGraphYAxisLabels = nullptr;
    QLabel *m_labelTickLabelFont = nullptr;
    FontSelector *m_graphTickLabelFont = nullptr;

    QCheckBox *m_cbGraphShowSpeed = nullptr;
    QLabel *m_labelSpeedLabelFont = nullptr;
    FontSelector *m_graphSpeedLabelFont = nullptr;
    QLabel *m_labelSpeedPosition = nullptr;
    QComboBox *m_comboSpeedPosition = nullptr;
    QCheckBox *m_cbGraphSpeedInside = nullptr;
    QLabel *m_labelTrafUnits = nullptr;
    QComboBox *m_comboTrafUnit = nullptr;

    std::array<LabelColor *, GraphColorCount> m_graphColors = {};
};

#endif // GRAPHOPTIONSAREA_H
