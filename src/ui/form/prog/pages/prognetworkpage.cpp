#include "prognetworkpage.h"

#include <QCheckBox>

#include <conf/app.h>
#include <conf/confrulemanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/toolbutton.h>
#include <form/controls/zonesselector.h>
#include <form/rule/ruleswindow.h>
#include <fortglobal.h>
#include <model/rulelistmodel.h>

using namespace Fort;

ProgNetworkPage::ProgNetworkPage(ProgramEditController *ctrl, QWidget *parent) :
    ProgBasePage(ctrl, parent)
{
    setupUi();
}

void ProgNetworkPage::onPageInitialize(const App &app)
{
    m_cbLanOnly->setChecked(app.lanOnly);
    m_btZones->setZones(app.zones.accept_mask);
    m_btZones->setUncheckedZones(app.zones.reject_mask);

    initializeRuleField(isSingleSelection());
}

void ProgNetworkPage::onRetranslateUi()
{
    m_cbLanOnly->setText(tr("Block Internet Traffic"));
    m_btZones->retranslateUi();

    m_editRuleName->setPlaceholderText(tr("Rule"));
    m_btSelectRule->setToolTip(tr("Select Rule"));
}

void ProgNetworkPage::initializeRuleField(bool isSingleSelection)
{
    const auto ruleId = app().ruleId;

    setCurrentRuleId(ruleId);

    m_editRuleName->setStartText(
            isSingleSelection ? confRuleManager()->ruleNameById(ruleId) : QString());
    m_editRuleName->setEnabled(isSingleSelection);
    m_editRuleName->setClearButtonEnabled(isSingleSelection);

    m_btSelectRule->setEnabled(isSingleSelection);
}

void ProgNetworkPage::setupUi()
{
    // Zones/Rule
    auto zonesRuleLayout = setupZonesRuleLayout();

    // Main Layout
    auto layout = new QVBoxLayout();
    layout->addLayout(zonesRuleLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addStretch();

    this->setLayout(layout);
}

QLayout *ProgNetworkPage::setupZonesRuleLayout()
{
    // LAN Only
    m_cbLanOnly = ControlUtil::createCheckBox(":/icons/hostname.png");

    // Zones
    m_btZones = new ZonesSelector();
    m_btZones->setIsTristate(true);

    // Rule
    auto ruleLayout = setupRuleLayout();

    auto layout = new QHBoxLayout();
    layout->addWidget(m_cbLanOnly);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btZones);
    layout->addStretch();
    layout->addLayout(ruleLayout, 1);

    return layout;
}

QLayout *ProgNetworkPage::setupRuleLayout()
{
    m_editRuleName = new LineEdit();
    m_editRuleName->setFocusPolicy(Qt::NoFocus);
    m_editRuleName->setContextMenuPolicy(Qt::PreventContextMenu);
    m_editRuleName->setMaximumWidth(300);

    connect(m_editRuleName, &QLineEdit::textEdited, this, [&](const QString &text) {
        if (text.isEmpty()) {
            setCurrentRuleId();
        }
    });

    // Select Rule
    m_btSelectRule = ControlUtil::createIconToolButton(":/icons/script.png", [&] {
        const quint16 ruleId = currentRuleId();
        if (ruleId != 0) {
            editRuleDialog(ruleId);
        } else {
            selectRuleDialog();
        }
    });

    auto layout = ControlUtil::createRowLayout(m_editRuleName, m_btSelectRule);
    layout->setSpacing(0);

    return layout;
}

void ProgNetworkPage::selectRuleDialog()
{
    auto rulesDialog = RulesWindow::showRulesDialog(Rule::AppRule, this);

    connect(rulesDialog, &RulesWindow::ruleSelected, this, [&](const RuleRow &ruleRow) {
        setCurrentRuleId(ruleRow.ruleId);
        m_editRuleName->setStartText(ruleRow.ruleName);
    });
}

void ProgNetworkPage::editRuleDialog(int ruleId)
{
    RulesWindow::showRuleEditDialog(ruleId, Rule::AppRule, this);
}

void ProgNetworkPage::fillApp(App &app) const
{
    app.lanOnly = m_cbLanOnly->isChecked();

    app.zones.accept_mask = m_btZones->zones();
    app.zones.reject_mask = m_btZones->uncheckedZones();

    app.ruleId = currentRuleId();
}
