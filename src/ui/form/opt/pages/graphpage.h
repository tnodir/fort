#ifndef GRAPHPAGE_H
#define GRAPHPAGE_H

#include "optbasepage.h"

class LabelColor;
class LabelSpin;

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
    void setupUi();
    QLayout *setupColumns();
    QLayout *setupColumn1();
    void setupGraphBox();
    void setupGraphCheckboxes();
    void setupGraphOptions();
    QLayout *setupColumn2();
    void setupColorsBox();
    void setupGraphColors();

private:
    QGroupBox *m_gbGraph = nullptr;
    QGroupBox *m_gbColors = nullptr;

    QCheckBox *m_cbGraphHideOnClose = nullptr;
    QCheckBox *m_cbGraphAlwaysOnTop = nullptr;
    QCheckBox *m_cbGraphFrameless = nullptr;
    QCheckBox *m_cbGraphClickThrough = nullptr;
    QCheckBox *m_cbGraphHideOnHover = nullptr;
    LabelSpin *m_graphOpacity = nullptr;
    LabelSpin *m_graphHoverOpacity = nullptr;
    LabelSpin *m_graphMaxSeconds = nullptr;

    LabelColor *m_graphColor = nullptr;
    LabelColor *m_graphColorIn = nullptr;
    LabelColor *m_graphColorOut = nullptr;
    LabelColor *m_graphAxisColor = nullptr;
    LabelColor *m_graphTickLabelColor = nullptr;
    LabelColor *m_graphLabelColor = nullptr;
    LabelColor *m_graphGridColor = nullptr;
};

#endif // GRAPHPAGE_H
