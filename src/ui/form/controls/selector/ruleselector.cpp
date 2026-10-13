#include "ruleselector.h"

#include <conf/confrulemanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/toolbutton.h>
#include <form/rule/ruleswindow.h>
#include <fortglobal.h>
#include <model/rulelistmodel.h>

using namespace Fort;

RuleSelector::RuleSelector(QWidget *parent) : QWidget(parent)
{
    setupUi();
    setupController();
}

void RuleSelector::setRuleId(quint16 ruleId)
{
    m_ruleId = ruleId;

    updateRuleName();
}

void RuleSelector::setShowRuleName(bool v)
{
    if (m_showRuleName == v)
        return;

    m_showRuleName = v;

    updateRuleName();
}

void RuleSelector::retranslateUi()
{
    m_editRuleName->setPlaceholderText(tr("Rule"));
    m_btSelectRule->setToolTip(tr("Select Rule"));
}

void RuleSelector::updateRuleName()
{
    const QString ruleName = showRuleName() ? confRuleManager()->ruleNameById(m_ruleId) : QString();

    m_editRuleName->setStartText(ruleName);
}

void RuleSelector::setupUi()
{
    m_editRuleName = new LineEdit();
    m_editRuleName->setFocusPolicy(Qt::NoFocus);
    m_editRuleName->setContextMenuPolicy(Qt::PreventContextMenu);
    m_editRuleName->setClearButtonEnabled(true);

    connect(m_editRuleName, &QLineEdit::textEdited, this, [&](const QString &text) {
        if (text.isEmpty()) {
            m_ruleId = 0;
        }
    });

    // Select Rule
    m_btSelectRule = ControlUtil::createIconToolButton(":/icons/script.png", [&] {
        if (m_ruleId != 0) {
            editRuleDialog();
        } else {
            selectRuleDialog();
        }
    });

    auto layout = ControlUtil::createRowLayout(m_editRuleName, m_btSelectRule);
    layout->setSpacing(0);

    this->setLayout(layout);
}

void RuleSelector::setupController()
{
    connect(confRuleManager(), &ConfRuleManager::ruleRemoved, this, [&](quint16 ruleId) {
        if (m_ruleId == ruleId) {
            setRuleId(0); // the deleted Rule's id can be reused
        }
    });
}

void RuleSelector::selectRuleDialog()
{
    auto rulesDialog = RulesWindow::showRulesDialog(ruleType(), this);

    connect(rulesDialog, &RulesWindow::ruleSelected, this,
            [&](const RuleRow &ruleRow) { setRuleId(ruleRow.ruleId); });
}

void RuleSelector::editRuleDialog()
{
    RulesWindow::showRuleEditDialog(m_ruleId, ruleType(), this);
}
