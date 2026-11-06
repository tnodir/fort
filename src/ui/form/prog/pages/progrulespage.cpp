#include "progrulespage.h"

#include <QVBoxLayout>

#include <common/fortconf.h>

#include <conf/app.h>
#include <conf/confrulemanager.h>
#include <conf/rule.h>
#include <form/controls/controlutil.h>
#include <form/controls/selector/terminatingruleselector.h>
#include <form/rule/rulesetarea.h>
#include <fortglobal.h>
#include <model/rulesetmodel.h>
#include <util/conf/confutil.h>

using namespace Fort;

ProgRulesPage::ProgRulesPage(ProgramEditController *ctrl, QWidget *parent) :
    ProgBasePage(ctrl, parent)
{
    setupUi();
}

int ProgRulesPage::rulesCount() const
{
    return m_ruleSetArea->ruleSetModel()->rowCount();
}

void ProgRulesPage::setTerminatingBlock()
{
    m_terminatingRuleSelector->setTerminate(true);
    m_terminatingRuleSelector->setTerminateActionTypeId(FORT_RULE_FILTER_ACTION_BLOCK);
}

void ProgRulesPage::fillApp(App &app) const
{
    app.ruleSet = m_ruleSetArea->ruleSetModel()->ruleSet();

    // The Terminating Rule's line of the Network Filters' text
    Rule rule;
    ConfUtil::parseAppFiltersText(app.filtersText, rule);
    m_terminatingRuleSelector->fillRule(rule);

    app.filtersText = ConfUtil::appFiltersText(rule);
}

void ProgRulesPage::onPageInitialize(const App &app)
{
    Rule rule;
    ConfUtil::parseAppFiltersText(app.filtersText, rule);
    rule.ruleSet = app.ruleSet;

    m_ruleSetArea->initialize(rule, confRuleManager()->ruleNamesByIds(rule.ruleSet));
    m_terminatingRuleSelector->initialize(rule);

    const bool isSingleSelection = this->isSingleSelection();

    m_ruleSetArea->setEnabled(isSingleSelection);
    m_terminatingRuleSelector->setEnabled(isSingleSelection);

    emit rulesCountChanged();
}

void ProgRulesPage::onRetranslateUi()
{
    m_ruleSetArea->retranslateUi();
    m_terminatingRuleSelector->retranslateUi();
}

void ProgRulesPage::setupUi()
{
    // Rule Set
    m_ruleSetArea = new RuleSetArea(Rule::assignedRuleTypes());

    connect(m_ruleSetArea->ruleSetModel(), &RuleSetModel::rowCountChanged, this,
            &ProgRulesPage::rulesCountChanged);

    // Terminating Rule
    m_terminatingRuleSelector = new TerminatingRuleSelector();

    // Main Layout
    auto layout = new QVBoxLayout();
    layout->addWidget(m_ruleSetArea, 1);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addWidget(m_terminatingRuleSelector);

    this->setLayout(layout);
}
