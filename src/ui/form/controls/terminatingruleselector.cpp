#include "terminatingruleselector.h"

#include <QCheckBox>
#include <QComboBox>

#include <form/controls/controlutil.h>

namespace {

enum ActionIndex : qint8 {
    ActionAllow = 0,
    ActionBlock,
    ActionDrop,
};

}

TerminatingRuleSelector::TerminatingRuleSelector(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

bool TerminatingRuleSelector::terminate() const
{
    return m_cbTerminate->isChecked();
}

void TerminatingRuleSelector::setTerminate(bool v)
{
    m_cbTerminate->setChecked(v);
}

bool TerminatingRuleSelector::terminateBlocked() const
{
    return m_comboAction->currentIndex() != ActionAllow;
}

bool TerminatingRuleSelector::terminateDrop() const
{
    return m_comboAction->currentIndex() == ActionDrop;
}

void TerminatingRuleSelector::setTerminateAction(bool blocked, bool drop)
{
    const int index = blocked ? (drop ? ActionDrop : ActionBlock) : ActionAllow;

    m_comboAction->setCurrentIndex(index);
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

    ControlUtil::setComboBoxTexts(m_comboAction, { tr("Allow"), tr("Block"), tr("Drop") });
    ControlUtil::setComboBoxIcons(
            m_comboAction, { ":/icons/accept.png", ":/icons/deny.png", ":/icons/cross.png" });

    m_cbAlert->setText(tr("Alert"));
}

void TerminatingRuleSelector::setupUi()
{
    // Action
    m_comboAction = ControlUtil::createComboBox({ QString(), QString(), QString() });
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
