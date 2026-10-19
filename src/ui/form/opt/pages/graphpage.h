#ifndef GRAPHPAGE_H
#define GRAPHPAGE_H

#include "optbasepage.h"

class LabelColor;
class LabelSpin;
class LabelSpinCombo;

class GraphPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit GraphPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

public slots:
    void onResetToDefault() override;

protected slots:
    void onRetranslateUi() override;

private:
    void retranslateFixedSpeedCombo();
    void retranslateGraphTypeCombo();

    void setupUi();
    QLayout *setupColumns();
    QLayout *setupColumn1();
    QLayout *setupColumn2();

    void setupGraphBox();
    void setupGraphCheckboxes();
    void setupGraphViewCheckboxes();
    QLayout *setupGraphShowLayout();
    void setupGraphShowCheckboxes();
    void setupGraphOptions();
    void setupGraphFixedSpeed();
    QLayout *setupGraphTypeLayout();
    void changeGraphType(int index);
    QLayout *setupTrafUnitsLayout();
    void changeTrafUnit(int index);
    void setupColorsBox();
    void setupGraphColors();
    void setupGraphColors1();
    void setupGraphColors2();
    void setupGeometryBox();
    void saveGraphWindowGeometry();
    void setupGeometryControls();
    LabelSpin *createGeometrySpin(int value, int min);
    void onGeometryEdited();

private:
    QGroupBox *m_gbGraph = nullptr;
    QGroupBox *m_gbColors = nullptr;
    QGroupBox *m_gbGeometry = nullptr;

    QCheckBox *m_cbGraphHideOnClose = nullptr;
    QCheckBox *m_cbGraphAlwaysOnTop = nullptr;
    QCheckBox *m_cbGraphFrameless = nullptr;
    QCheckBox *m_cbGraphClickThrough = nullptr;
    QCheckBox *m_cbGraphHideOnHover = nullptr;
    QCheckBox *m_cbGraphHideYAxisLabels = nullptr;
    QCheckBox *m_cbGraphShowSpeed = nullptr;
    QCheckBox *m_cbGraphAnimation = nullptr;
    QCheckBox *m_cbGraphAxisTicks = nullptr;
    QLabel *m_labelGraphShow = nullptr;
    QCheckBox *m_cbGraphShowTotal = nullptr;
    QCheckBox *m_cbGraphShowIn = nullptr;
    QCheckBox *m_cbGraphShowOut = nullptr;
    LabelSpin *m_graphOpacity = nullptr;
    LabelSpin *m_graphHoverOpacity = nullptr;
    LabelSpin *m_graphTickLabelSize = nullptr;
    LabelSpin *m_graphBarWidth = nullptr;
    LabelSpin *m_graphMaxSeconds = nullptr;
    LabelSpinCombo *m_graphFixedSpeed = nullptr;
    QLabel *m_labelGraphType = nullptr;
    QComboBox *m_comboGraphType = nullptr;
    QLabel *m_traphUnits = nullptr;
    QComboBox *m_comboTrafUnit = nullptr;

    LabelColor *m_graphColor = nullptr;
    LabelColor *m_graphBorderColor = nullptr;
    LabelColor *m_graphColorIn = nullptr;
    LabelColor *m_graphColorOut = nullptr;
    LabelColor *m_graphTotalColor = nullptr;
    LabelColor *m_graphAxisColor = nullptr;
    LabelColor *m_graphTickLabelColor = nullptr;
    LabelColor *m_graphLabelColor = nullptr;
    LabelColor *m_graphGridColor = nullptr;

    QCheckBox *m_cbGraphMaximized = nullptr;
    LabelSpin *m_graphX = nullptr;
    LabelSpin *m_graphY = nullptr;
    LabelSpin *m_graphWidth = nullptr;
    LabelSpin *m_graphHeight = nullptr;
};

#endif // GRAPHPAGE_H
