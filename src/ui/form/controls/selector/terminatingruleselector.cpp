#include "terminatingruleselector.h"

#include <QCheckBox>
#include <QComboBox>

#include <algorithm>

#include <common/fortconf.h>

#include <conf/rule.h>
#include <form/controls/controlutil.h>

namespace {

// By the Action combo box's index
inline constexpr quint8 actionTypeIds[] = {
    FORT_RULE_FILTER_ACTION_ALLOW,
    FORT_RULE_FILTER_ACTION_BLOCK,
    FORT_RULE_FILTER_ACTION_ASK,
    FORT_RULE_FILTER_ACTION_DROP,
};

}

TerminatingRuleSelector::TerminatingRuleSelector(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void TerminatingRuleSelector::initialize(const Rule &rule)
{
    setTerminate(rule.terminate);
    setTerminateActionTypeId(rule.terminateActionTypeId());
    setTerminateAlert(rule.terminateAlert);
}

void TerminatingRuleSelector::fillRule(Rule &rule) const
{
    rule.terminate = terminate();
    rule.setTerminateActionTypeId(terminateActionTypeId());
    rule.terminateAlert = terminateAlert();
}

bool TerminatingRuleSelector::terminate() const
{
    return m_cbTerminate->isChecked();
}

void TerminatingRuleSelector::setTerminate(bool v)
{
    m_cbTerminate->setChecked(v);
}

quint8 TerminatingRuleSelector::terminateActionTypeId() const
{
    return actionTypeIds[m_comboAction->currentIndex()];
}

void TerminatingRuleSelector::setTerminateActionTypeId(quint8 actionTypeId)
{
    const auto it = std::find(std::begin(actionTypeIds), std::end(actionTypeIds), actionTypeId);

    m_comboAction->setCurrentIndex(int(it - std::begin(actionTypeIds)));
}

bool TerminatingRuleSelector::terminateAlert() const
{
    return m_cbAlert->isChecked();
}

void TerminatingRuleSelector::setTerminateAlert(bool v)
{
    m_cbAlert->setChecked(v);
}

void TerminatingRuleSelector::retranslateUi()
{
    m_cbTerminate->setText(tr("Terminating Rule:"));

    ControlUtil::setComboBoxTexts(
            m_comboAction, { tr("Allow"), tr("Block"), tr("Ask"), tr("Drop") });
    ControlUtil::setComboBoxIcons(m_comboAction,
            { ":/icons/accept.png", ":/icons/deny.png", ":/icons/help.png", ":/icons/cross.png" });

    m_cbAlert->setText(tr("Alert"));
}

void TerminatingRuleSelector::setupUi()
{
    // Action
    m_comboAction = ControlUtil::createComboBox({ QString(), QString(), QString(), QString() });
    m_comboAction->setMinimumWidth(100);

    // Alert
    m_cbAlert = ControlUtil::createCheckBox(":/icons/error.png");

    // Terminate Check Box
    setupCbTerminate();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbTerminate, m_comboAction, m_cbAlert, /*stretch*/ nullptr }, /*margin=*/0);

    this->setLayout(layout);
}

void TerminatingRuleSelector::setupCbTerminate()
{
    m_cbTerminate = new QCheckBox();

    const auto refreshTerminateEnabled = [&](bool checked) {
        m_comboAction->setEnabled(checked);
        m_cbAlert->setEnabled(checked);
    };

    refreshTerminateEnabled(false);

    connect(m_cbTerminate, &QCheckBox::toggled, this, refreshTerminateEnabled);
}
