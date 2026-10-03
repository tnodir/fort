#include "prognetworkpage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

#include <conf/app.h>
#include <form/controls/controlutil.h>
#include <form/controls/ruleselector.h>
#include <form/controls/zonesselector.h>
#include <fortglobal.h>
#include <model/speedlimitlistmodel.h>

using namespace Fort;

namespace {

void selectComboSpeedLimit(QComboBox *combo, quint8 limitId)
{
    const int index = combo->findData(int(limitId));

    combo->setCurrentIndex(index < 0 ? 0 : index); // the deleted Speed Limit is "No Limit"
}

}

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
    initializeSpeedLimitFields();
}

void ProgNetworkPage::onRetranslateUi()
{
    m_cbLanOnly->setText(tr("Block Internet Traffic"));
    m_btZones->retranslateUi();

    m_ruleSelector->retranslateUi();

    m_labelSpeedLimitIn->setText(tr("Download:"));
    m_labelSpeedLimitOut->setText(tr("Upload:"));

    retranslateSpeedLimitFields();
}

void ProgNetworkPage::initializeRuleField(bool isSingleSelection)
{
    m_ruleSelector->setShowRuleName(isSingleSelection);
    m_ruleSelector->setRuleId(app().ruleId);
    m_ruleSelector->setEnabled(isSingleSelection);
}

void ProgNetworkPage::initializeSpeedLimitFields()
{
    const FORT_SPEED_LIMIT_IDS speedLimits = app().speedLimits;

    selectComboSpeedLimit(m_comboSpeedLimitIn, speedLimits.in_limit_id);
    selectComboSpeedLimit(m_comboSpeedLimitOut, speedLimits.out_limit_id);
}

void ProgNetworkPage::retranslateSpeedLimitFields()
{
    const QString noLimitText = tr("No Limit");

    m_comboSpeedLimitIn->setItemText(0, noLimitText);
    m_comboSpeedLimitOut->setItemText(0, noLimitText);
}

void ProgNetworkPage::setupUi()
{
    // Speed Limits
    auto speedLimitsLayout = setupSpeedLimitsLayout();

    // Zones/Rule
    auto zonesRuleLayout = setupZonesRuleLayout();

    // Main Layout
    auto layout = new QVBoxLayout();
    layout->addLayout(speedLimitsLayout);
    layout->addWidget(ControlUtil::createSeparator());
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

QLayout *ProgNetworkPage::setupSpeedLimitsLayout()
{
    // Download
    m_labelSpeedLimitIn = ControlUtil::createLabel();

    m_comboSpeedLimitIn = ControlUtil::createComboBox();
    m_comboSpeedLimitIn->setMinimumWidth(150);

    // Upload
    m_labelSpeedLimitOut = ControlUtil::createLabel();

    m_comboSpeedLimitOut = ControlUtil::createComboBox();
    m_comboSpeedLimitOut->setMinimumWidth(150);

    setupSpeedLimitsChanged();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_labelSpeedLimitIn, m_comboSpeedLimitIn,
            ControlUtil::createVSeparator(), m_labelSpeedLimitOut, m_comboSpeedLimitOut,
            /*stretch*/ nullptr });

    return layout;
}

void ProgNetworkPage::setupSpeedLimitsChanged()
{
    updateSpeedLimitCombos();

    auto speedLimitListModel = Fort::speedLimitListModel();

    connect(speedLimitListModel, &SpeedLimitListModel::modelReset, this,
            &ProgNetworkPage::updateSpeedLimitCombos);
    connect(speedLimitListModel, &SpeedLimitListModel::dataChanged, this,
            [&](const QModelIndex &topLeft) {
                // Skip the queues' status updates
                if (topLeft.column() == int(SpeedLimitListColumn::Name)) {
                    updateSpeedLimitCombos();
                }
            });
}

void ProgNetworkPage::updateSpeedLimitCombos()
{
    const quint8 inLimitId = m_comboSpeedLimitIn->currentData().toUInt();
    const quint8 outLimitId = m_comboSpeedLimitOut->currentData().toUInt();

    m_comboSpeedLimitIn->clear();
    m_comboSpeedLimitOut->clear();

    m_comboSpeedLimitIn->addItem(QString(), 0);
    m_comboSpeedLimitOut->addItem(QString(), 0);

    const auto speedLimitListModel = Fort::speedLimitListModel();

    const int speedLimitsCount = speedLimitListModel->rowCount();
    for (int row = 0; row < speedLimitsCount; ++row) {
        const auto &speedLimitRow = speedLimitListModel->speedLimitRowAt(row);

        QComboBox *combo = speedLimitRow.inbound ? m_comboSpeedLimitIn : m_comboSpeedLimitOut;

        combo->addItem(speedLimitRow.menuLabel(), int(speedLimitRow.limitId));
    }

    retranslateSpeedLimitFields();

    selectComboSpeedLimit(m_comboSpeedLimitIn, inLimitId);
    selectComboSpeedLimit(m_comboSpeedLimitOut, outLimitId);
}

void ProgNetworkPage::fillApp(App &app) const
{
    app.lanOnly = m_cbLanOnly->isChecked();

    app.zones.accept_mask = m_btZones->zones();
    app.zones.reject_mask = m_btZones->uncheckedZones();

    app.ruleId = m_ruleSelector->ruleId();

    app.speedLimits.in_limit_id = m_comboSpeedLimitIn->currentData().toUInt();
    app.speedLimits.out_limit_id = m_comboSpeedLimitOut->currentData().toUInt();
}
