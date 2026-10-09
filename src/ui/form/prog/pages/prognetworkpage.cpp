#include "prognetworkpage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QToolButton>

#include <common/fortconf.h>

#include <conf/app.h>
#include <conf/rule.h>
#include <form/controls/controlutil.h>
#include <form/controls/listview.h>
#include <form/controls/ruleselector.h>
#include <form/controls/terminatingruleselector.h>
#include <form/controls/zonesselector.h>
#include <form/dialog/dialogutil.h>
#include <form/rule/filtereditdialog.h>
#include <fortglobal.h>
#include <model/speedlimitlistmodel.h>
#include <util/conf/confutil.h>
#include <util/model/stringlistmodel.h>

using namespace Fort;

namespace {

void selectComboSpeedLimit(QComboBox *combo, quint8 limitId)
{
    const int index = combo->findData(int(limitId));

    combo->setCurrentIndex(index < 0 ? 0 : index); // the deleted Speed Limit is "No Limit"
}

QComboBox *createComboSpeedLimit(QCheckBox *cb)
{
    auto c = ControlUtil::createComboBox();
    c->setMinimumWidth(150);
    c->setEnabled(false);

    QObject::connect(cb, &QCheckBox::toggled, c, &QComboBox::setEnabled);

    return c;
}

}

ProgNetworkPage::ProgNetworkPage(ProgramEditController *ctrl, QWidget *parent) :
    ProgBasePage(ctrl, parent), m_filterListModel(new StringListModel(this))
{
    setupUi();
}

void ProgNetworkPage::onPageInitialize(const App &app)
{
    m_cbBlockInbound->setChecked(app.blockInbound);
    m_cbBlockOutbound->setChecked(app.blockOutbound);
    m_cbLanOnly->setChecked(app.lanOnly);
    m_btZones->setZones(app.zones.accept_mask);
    m_btZones->setUncheckedZones(app.zones.reject_mask);

    initializeRuleField(isSingleSelection());
    initializeSpeedLimitFields();
    initializeFilters(isSingleSelection());
}

void ProgNetworkPage::onRetranslateUi()
{
    m_labelBlock->setText(tr("Block:"));
    m_cbBlockInbound->setText(tr("In"));
    m_cbBlockInbound->setToolTip(tr("Block Inbound Connections"));
    m_cbBlockOutbound->setText(tr("Out"));
    m_cbBlockOutbound->setToolTip(tr("Block Outbound Connections"));
    m_cbLanOnly->setText(tr("Internet"));
    m_cbLanOnly->setToolTip(tr("Block Internet Traffic"));
    m_btZones->retranslateUi();

    m_ruleSelector->retranslateUi();

    m_cbSpeedLimitIn->setText(tr("Download:"));
    m_cbSpeedLimitOut->setText(tr("Upload:"));

    retranslateSpeedLimitFields();

    retranslateFilters();
}

void ProgNetworkPage::initializeRuleField(bool isSingleSelection)
{
    m_ruleSelector->setShowRuleName(isSingleSelection);
    m_ruleSelector->setRuleId(app().ruleId);
    m_ruleSelector->setEnabled(isSingleSelection);
}

void ProgNetworkPage::initializeSpeedLimitFields()
{
    const App &app = this->app();
    const FORT_SPEED_LIMIT_IDS speedLimits = app.speedLimits;

    m_cbSpeedLimitIn->setChecked(app.inLimitEnabled);
    m_cbSpeedLimitOut->setChecked(app.outLimitEnabled);

    selectComboSpeedLimit(m_comboSpeedLimitIn, speedLimits.in_limit_id);
    selectComboSpeedLimit(m_comboSpeedLimitOut, speedLimits.out_limit_id);
}

void ProgNetworkPage::initializeFilters(bool isSingleSelection)
{
    Rule rule;
    ConfUtil::parseAppFiltersText(app().filtersText, rule);

    m_filterListModel->setList(rule.ruleText.split('\n', Qt::SkipEmptyParts));
    m_terminatingRuleSelector->initialize(rule);

    m_btAddFilter->setEnabled(isSingleSelection);
    m_filterListView->setEnabled(isSingleSelection);
    m_terminatingRuleSelector->setEnabled(isSingleSelection);
}

void ProgNetworkPage::retranslateSpeedLimitFields()
{
    const QString noLimitText = tr("No Limit");

    m_comboSpeedLimitIn->setItemText(0, noLimitText);
    m_comboSpeedLimitOut->setItemText(0, noLimitText);
}

void ProgNetworkPage::retranslateFilters()
{
    m_btAddFilter->setText(tr("Add Filter"));
    m_btRemoveFilter->setText(tr("Remove"));
    m_btEditFilter->setText(tr("Edit"));
    m_btUpFilter->setToolTip(tr("Move Up"));
    m_btDownFilter->setToolTip(tr("Move Down"));

    m_terminatingRuleSelector->retranslateUi();
}

void ProgNetworkPage::setupUi()
{
    // Speed Limits
    auto speedLimitsLayout = setupSpeedLimitsLayout();

    // Zones/Rule
    auto zonesRuleLayout = setupZonesRuleLayout();

    // Filters Header
    auto filtersHeaderLayout = setupFiltersHeaderLayout();

    // Filter List View
    setupFilterListView();

    // Actions on filter list view's current changed
    setupFilterListViewChanged();

    // Terminating Rule
    m_terminatingRuleSelector = new TerminatingRuleSelector();

    // Main Layout
    auto layout = new QVBoxLayout();
    layout->addLayout(speedLimitsLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(zonesRuleLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(filtersHeaderLayout);
    layout->addWidget(m_filterListView, 1);
    layout->addWidget(m_terminatingRuleSelector);

    this->setLayout(layout);
}

QLayout *ProgNetworkPage::setupZonesRuleLayout()
{
    // Block
    m_labelBlock = ControlUtil::createLabel();

    // Block Inbound
    m_cbBlockInbound = ControlUtil::createCheckBox();

    // Block Outbound
    m_cbBlockOutbound = ControlUtil::createCheckBox();

    // LAN Only
    m_cbLanOnly = ControlUtil::createCheckBox();

    // Zones
    m_btZones = new ZonesSelector();
    m_btZones->setIsTristate(true);

    // Rule
    m_ruleSelector = new RuleSelector();
    m_ruleSelector->setMaximumWidth(300);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_labelBlock);
    layout->addWidget(m_cbBlockInbound);
    layout->addWidget(m_cbBlockOutbound);
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
    m_cbSpeedLimitIn = ControlUtil::createCheckBox(":/icons/green_down.png");
    m_comboSpeedLimitIn = createComboSpeedLimit(m_cbSpeedLimitIn);

    // Upload
    m_cbSpeedLimitOut = ControlUtil::createCheckBox(":/icons/blue_up.png");
    m_comboSpeedLimitOut = createComboSpeedLimit(m_cbSpeedLimitOut);

    setupSpeedLimitsChanged();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbSpeedLimitIn, m_comboSpeedLimitIn,
            /*stretch*/ nullptr, m_cbSpeedLimitOut, m_comboSpeedLimitOut, /*stretch*/ nullptr });

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

QLayout *ProgNetworkPage::setupFiltersHeaderLayout()
{
    m_btAddFilter =
            ControlUtil::createFlatToolButton(":/icons/add.png", [&] { openFilterEditForm(); });
    m_btRemoveFilter = ControlUtil::createFlatToolButton(
            ":/icons/delete.png", [&] { m_filterListModel->remove(filterListCurrentIndex()); });
    m_btEditFilter =
            ControlUtil::createFlatToolButton(":/icons/pencil.png", [&] { editCurrentFilter(); });
    m_btUpFilter = ControlUtil::createIconToolButton(
            ":/icons/bullet_arrow_up.png", [&] { moveCurrentFilter(-1); });
    m_btDownFilter = ControlUtil::createIconToolButton(
            ":/icons/bullet_arrow_down.png", [&] { moveCurrentFilter(1); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btAddFilter, m_btRemoveFilter,
            m_btEditFilter, ControlUtil::createVSeparator(), m_btUpFilter, m_btDownFilter,
            /*stretch*/ nullptr });

    return layout;
}

void ProgNetworkPage::setupFilterListView()
{
    m_filterListView = new ListView();
    m_filterListView->setFlow(QListView::TopToBottom);
    m_filterListView->setViewMode(QListView::ListMode);
    m_filterListView->setUniformItemSizes(true);
    m_filterListView->setAlternatingRowColors(true);

    // Don't enlarge the dialog by the list's size hint, but stretch the list with it
    m_filterListView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    m_filterListView->setMinimumHeight(40);

    m_filterListView->setModel(m_filterListModel);

    connect(m_filterListView, &ListView::doubleClicked, m_btEditFilter, &QToolButton::click);
}

void ProgNetworkPage::setupFilterListViewChanged()
{
    const auto refreshFilterListViewChanged = [&] {
        const bool filterSelected = (filterListCurrentIndex() >= 0);
        m_btRemoveFilter->setEnabled(filterSelected);
        m_btEditFilter->setEnabled(filterSelected);
        m_btUpFilter->setEnabled(filterSelected);
        m_btDownFilter->setEnabled(filterSelected);
    };

    refreshFilterListViewChanged();

    connect(m_filterListView, &ListView::currentIndexChanged, this, refreshFilterListViewChanged);
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

        combo->addItem(SpeedLimitListModel::menuLabel(speedLimitRow), int(speedLimitRow.limitId));
    }

    retranslateSpeedLimitFields();

    selectComboSpeedLimit(m_comboSpeedLimitIn, inLimitId);
    selectComboSpeedLimit(m_comboSpeedLimitOut, outLimitId);
}

void ProgNetworkPage::fillApp(App &app) const
{
    app.blockInbound = m_cbBlockInbound->isChecked();
    app.blockOutbound = m_cbBlockOutbound->isChecked();
    app.lanOnly = m_cbLanOnly->isChecked();

    app.zones.accept_mask = m_btZones->zones();
    app.zones.reject_mask = m_btZones->uncheckedZones();

    app.ruleId = m_ruleSelector->ruleId();

    app.inLimitEnabled = m_cbSpeedLimitIn->isChecked();
    app.outLimitEnabled = m_cbSpeedLimitOut->isChecked();

    app.speedLimits.in_limit_id = m_comboSpeedLimitIn->currentData().toUInt();
    app.speedLimits.out_limit_id = m_comboSpeedLimitOut->currentData().toUInt();

    Rule rule;
    rule.ruleText = m_filterListModel->list().join('\n');
    m_terminatingRuleSelector->fillRule(rule);

    app.filtersText = ConfUtil::appFiltersText(rule);
}

int ProgNetworkPage::filterListCurrentIndex() const
{
    return m_filterListView->currentRow();
}

void ProgNetworkPage::openFilterEditForm(const FilterLineText &lineText, int row)
{
    auto w = new FilterEditDialog(/*isRuleFilter=*/false, this);
    ControlUtil::deleteOnClose(w);

    connect(w, &FilterEditDialog::filterSaved, this, &ProgNetworkPage::saveFilter);

    w->initialize(lineText, row);

    DialogUtil::showDialog(w);
}

void ProgNetworkPage::openConnFilterForm(const Conn &conn)
{
    openFilterEditForm(FilterLineText(conn));
}

void ProgNetworkPage::editCurrentFilter()
{
    const int row = filterListCurrentIndex();
    if (row < 0)
        return;

    openFilterEditForm(m_filterListModel->list().at(row), row);
}

void ProgNetworkPage::moveCurrentFilter(int offset)
{
    const int row = filterListCurrentIndex();
    const int toRow = row + offset;

    if (m_filterListModel->canMove(row, toRow)) {
        m_filterListModel->move(row, toRow);
    }
}

void ProgNetworkPage::saveFilter(const QString &filterText, int row)
{
    if (row < 0) {
        row = m_filterListModel->rowCount();
        m_filterListModel->insert(filterText, row);
    } else {
        m_filterListModel->replace(filterText, row);
    }

    m_filterListView->setCurrentIndex(m_filterListModel->index(row));
}
