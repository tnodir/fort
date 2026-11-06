#ifndef PROGRULESPAGE_H
#define PROGRULESPAGE_H

#include "progbasepage.h"

class RuleSetArea;
class TerminatingRuleSelector;

class ProgRulesPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgRulesPage(ProgramEditController *ctrl = nullptr, QWidget *parent = nullptr);

    QString helpSection() const override { return "rules-tab"; }

    int rulesCount() const;

    void setTerminatingBlock();

signals:
    void rulesCountChanged();

public slots:
    void fillApp(App &app) const override;

protected slots:
    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void setupUi();

private:
    RuleSetArea *m_ruleSetArea = nullptr;
    TerminatingRuleSelector *m_terminatingRuleSelector = nullptr;
};

#endif // PROGRULESPAGE_H
