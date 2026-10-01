#include "prognetworkpage.h"

#include <QCheckBox>

#include <conf/app.h>
#include <form/controls/controlutil.h>
#include <form/controls/ruleselector.h>
#include <form/controls/zonesselector.h>

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

    m_ruleSelector->retranslateUi();
}

void ProgNetworkPage::initializeRuleField(bool isSingleSelection)
{
    m_ruleSelector->setShowRuleName(isSingleSelection);
    m_ruleSelector->setRuleId(app().ruleId);
    m_ruleSelector->setEnabled(isSingleSelection);
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
    m_ruleSelector = new RuleSelector();
    m_ruleSelector->setMaximumWidth(300);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_cbLanOnly);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btZones);
    layout->addStretch();
    layout->addWidget(m_ruleSelector, 1);

    return layout;
}

void ProgNetworkPage::fillApp(App &app) const
{
    app.lanOnly = m_cbLanOnly->isChecked();

    app.zones.accept_mask = m_btZones->zones();
    app.zones.reject_mask = m_btZones->uncheckedZones();

    app.ruleId = m_ruleSelector->ruleId();
}
