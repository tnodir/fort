#ifndef PROGNETWORKPAGE_H
#define PROGNETWORKPAGE_H

#include "progbasepage.h"

class RuleSelector;
class ZonesSelector;

class ProgNetworkPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgNetworkPage(ProgramEditController *ctrl = nullptr, QWidget *parent = nullptr);

public slots:
    void fillApp(App &app) const override;

protected slots:
    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void initializeRuleField(bool isSingleSelection);

    void setupUi();
    QLayout *setupZonesRuleLayout();

private:
    QCheckBox *m_cbLanOnly = nullptr;
    ZonesSelector *m_btZones = nullptr;
    RuleSelector *m_ruleSelector = nullptr;
};

#endif // PROGNETWORKPAGE_H
