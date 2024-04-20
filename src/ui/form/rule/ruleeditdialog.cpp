#include "ruleeditdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confrulemanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/listview.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/zonesselector.h>
#include <manager/windowmanager.h>
#include <model/rulesetmodel.h>
#include <util/conf/confutil.h>
#include <util/iconcache.h>
#include <util/net/netutil.h>

#include "rulescontroller.h"
#include "ruleswindow.h"

RuleEditDialog::RuleEditDialog(RulesController *ctrl, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl), m_ruleSetModel(new RuleSetModel(this))
{
    setupUi();
    setupController();
}

ConfRuleManager *RuleEditDialog::confRuleManager() const
{
    return ctrl()->confRuleManager();
}

WindowManager *RuleEditDialog::windowManager() const
{
    return ctrl()->windowManager();
}

void RuleEditDialog::initialize(const RuleRow &ruleRow)
{
    m_ruleRow = ruleRow;

    initializeRuleSet();

    retranslateUi();

    m_editName->setStartText(ruleRow.ruleName);
    m_editName->setClearButtonEnabled(true);

    m_labelEditNotes->setPixmap(IconCache::file(":/icons/script.png"));
    m_editNotes->setText(ruleRow.notes);

    m_labelRuleType->setText(tr("Type:"));
    m_comboRuleType->setCurrentIndex(ruleRow.ruleType);
    m_comboRuleType->setEnabled(isEmpty());

    m_cbEnabled->setChecked(ruleRow.enabled);

    m_rbAllow->setChecked(!ruleRow.blocked);
    m_rbBlock->setChecked(ruleRow.blocked);

    m_editRuleText->setText(ruleRow.ruleText);

    m_cbExclusive->setChecked(ruleRow.exclusive);

    m_btZones->setZones(ruleRow.acceptZones);
    m_btZones->setUncheckedZones(ruleRow.rejectZones);

    initializeFocus();
}

void RuleEditDialog::initializeRuleSet()
{
    QStringList ruleSetNames;

    confRuleManager()->loadRuleSet(m_ruleRow, ruleSetNames);

    ruleSetModel()->initialize(m_ruleRow, ruleSetNames);

    updateRuleSetViewVisible();
}

void RuleEditDialog::initializeFocus()
{
    if (isEmpty()) {
        m_editName->setFocus();
    } else {
        m_editRuleText->setFocus();
    }
}

void RuleEditDialog::setupController()
{
    connect(ctrl(), &RulesController::retranslateUi, this, &RuleEditDialog::retranslateUi);
}

void RuleEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelEditName->setText(tr("Name:"));
    m_editNotes->setPlaceholderText(tr("Notes"));

    retranslateComboRuleType();

    m_cbEnabled->setText(tr("Enabled"));

    m_rbAllow->setText(tr("Allow"));
    m_rbBlock->setText(tr("Block"));

    m_cbExclusive->setText(tr("Exclusive"));
    m_btZones->retranslateUi();

    retranslateRulePlaceholderText();

    m_btAddPresetRule->setText(tr("Add Preset Rule"));
    m_btRemovePresetRule->setText(tr("Remove"));
    m_btUpPresetRule->setToolTip(tr("Move Up"));
    m_btDownPresetRule->setToolTip(tr("Move Down"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Rule"));
}

void RuleEditDialog::retranslateComboRuleType()
{
    ControlUtil::setComboBoxTexts(m_comboRuleType, RuleListModel::ruleTypeNames());
}

void RuleEditDialog::retranslateRulePlaceholderText()
{
    const auto placeholderText = tr("# Examples:")
            // IP-Address:Port
            + '\n' + tr("# IP address and port:")
            + "\n1.1.1.1:udp(43)"
              "\n(1.1.1.1-8.8.8.8):(43,80-8080)"
              "\n\n!!! UNDER CONSTRUCTION !!!";

    m_editRuleText->setPlaceholderText(placeholderText);
}

void RuleEditDialog::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Size Grip
    this->setSizeGripEnabled(true);

    // Size
    this->setMinimumWidth(400);
}

QLayout *RuleEditDialog::setupMainLayout()
{
    // Form Layout
    auto formLayout = setupFormLayout();

    // Allow/Block Actions Layout
    auto actionsLayout = setupActionsLayout();

    // Zones Layout
    auto zonesLayout = setupZonesLayout();

    // Rule Text
    m_editRuleText = new PlainTextEdit();

    // TODO: Implement Rules
    m_editRuleText->setEnabled(false);

    // RuleSet Header
    auto ruleSetHeaderLayout = setupRuleSetHeaderLayout();

    // RuleSet View
    setupRuleSetView();

    // Actions on rule set view's current changed
    setupRuleSetViewChanged();

    // OK/Cancel
    auto buttonsLayout = setupButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addStretch();
    layout->addLayout(actionsLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(zonesLayout);
    layout->addWidget(m_editRuleText);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(ruleSetHeaderLayout);
    layout->addWidget(m_ruleSetView);
    layout->addStretch();
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *RuleEditDialog::setupFormLayout()
{
    auto layout = new QFormLayout();
    layout->setHorizontalSpacing(10);

    // Name
    m_editName = new LineEdit();
    m_editName->setMaxLength(1024);

    layout->addRow("Name:", m_editName);
    m_labelEditName = ControlUtil::formRowLabel(layout, m_editName);

    // Notes
    m_editNotes = new PlainTextEdit();
    m_editNotes->setFixedHeight(40);

    layout->addRow("Notes:", m_editNotes);
    m_labelEditNotes = ControlUtil::formRowLabel(layout, m_editNotes);
    m_labelEditNotes->setScaledContents(true);
    m_labelEditNotes->setFixedSize(32, 32);

    // Rule Type
    m_comboRuleType = ControlUtil::createComboBox();
    m_comboRuleType->setMinimumWidth(100);

    layout->addRow("Type:", m_comboRuleType);
    m_labelRuleType = ControlUtil::formRowLabel(layout, m_comboRuleType);

    // Enabled
    m_cbEnabled = new QCheckBox();

    layout->addRow(QString(), m_cbEnabled);

    return layout;
}

QLayout *RuleEditDialog::setupActionsLayout()
{
    // Allow
    m_rbAllow = new QRadioButton();
    m_rbAllow->setIcon(IconCache::icon(":/icons/accept.png"));
    m_rbAllow->setChecked(true);

    // Block
    m_rbBlock = new QRadioButton();
    m_rbBlock->setIcon(IconCache::icon(":/icons/deny.png"));

    auto layout = ControlUtil::createHLayoutByWidgets(
            { /*stretch*/ nullptr, m_rbAllow, m_rbBlock, /*stretch*/ nullptr });
    layout->setSpacing(20);

    return layout;
}

QLayout *RuleEditDialog::setupZonesLayout()
{
    // Exclusive
    m_cbExclusive = new QCheckBox();

    // Zones
    m_btZones = new ZonesSelector();
    m_btZones->setIsTristate(true);
    m_btZones->setMaxZoneCount(32); // sync with driver's FORT_CONF_RULE_ZONES

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbExclusive, ControlUtil::createVSeparator(), m_btZones, /*stretch*/ nullptr });

    return layout;
}

QLayout *RuleEditDialog::setupRuleSetHeaderLayout()
{
    m_btAddPresetRule =
            ControlUtil::createFlatToolButton(":/icons/add.png", [&] { selectPresetRuleDialog(); });
    m_btRemovePresetRule = ControlUtil::createFlatToolButton(
            ":/icons/delete.png", [&] { ruleSetModel()->remove(ruleSetCurrentIndex()); });
    m_btUpPresetRule = ControlUtil::createIconToolButton(
            ":/icons/bullet_arrow_up.png", [&] { ruleSetModel()->moveUp(ruleSetCurrentIndex()); });
    m_btDownPresetRule = ControlUtil::createIconToolButton(":/icons/bullet_arrow_down.png",
            [&] { ruleSetModel()->moveDown(ruleSetCurrentIndex()); });

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btAddPresetRule, m_btRemovePresetRule, ControlUtil::createVSeparator(),
                    m_btUpPresetRule, m_btDownPresetRule, /*stretch*/ nullptr });

    return layout;
}

void RuleEditDialog::setupRuleSetView()
{
    m_ruleSetView = new ListView();
    m_ruleSetView->setFlow(QListView::TopToBottom);
    m_ruleSetView->setViewMode(QListView::ListMode);
    m_ruleSetView->setUniformItemSizes(true);
    m_ruleSetView->setAlternatingRowColors(true);

    m_ruleSetView->setModel(ruleSetModel());

    connect(ruleSetModel(), &RuleSetModel::rowCountChanged, this,
            &RuleEditDialog::updateRuleSetViewVisible);
}

QLayout *RuleEditDialog::setupButtons()
{
    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] {
        if (save()) {
            this->close();
        }
    });
    m_btOk->setDefault(true);

    // Cancel
    m_btCancel = new QPushButton();
    connect(m_btCancel, &QAbstractButton::clicked, this, &QWidget::close);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btOk, 1, Qt::AlignRight);
    layout->addWidget(m_btCancel);

    return layout;
}

void RuleEditDialog::setupRuleSetViewChanged()
{
    const auto refreshRuleSetViewChanged = [&] {
        const bool ruleSelected = (ruleSetCurrentIndex() >= 0);
        m_btRemovePresetRule->setEnabled(ruleSelected);
        m_btUpPresetRule->setEnabled(ruleSelected);
        m_btDownPresetRule->setEnabled(ruleSelected);
    };

    refreshRuleSetViewChanged();

    connect(m_ruleSetView, &ListView::currentIndexChanged, this, refreshRuleSetViewChanged);
}

void RuleEditDialog::updateRuleSetViewVisible()
{
    const int ruleSetSize = ruleSetModel()->rowCount();

    m_ruleSetView->setVisible(ruleSetSize > 0);
}

int RuleEditDialog::ruleSetCurrentIndex() const
{
    return m_ruleSetView->currentRow();
}

bool RuleEditDialog::save()
{
    if (!validateFields())
        return false;

    Rule rule;
    fillRule(rule);

    // Add new zone
    if (isEmpty()) {
        return ctrl()->addOrUpdateRule(rule);
    }

    // Edit selected zone
    return saveRule(rule);
}

bool RuleEditDialog::saveRule(Rule &rule)
{
    if (rule.ruleSetEdited || !rule.isOptionsEqual(m_ruleRow)) {
        rule.ruleId = m_ruleRow.ruleId;

        return ctrl()->addOrUpdateRule(rule);
    }

    if (!rule.isNameEqual(m_ruleRow)) {
        return ctrl()->updateRuleName(m_ruleRow.ruleId, rule.ruleName);
    }

    return true;
}

bool RuleEditDialog::validateFields() const
{
    // Name
    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    // Global Rules Count
    const auto ruleType = Rule::RuleType(m_comboRuleType->currentIndex());
    if (ruleType == Rule::GlobalBeforeAppsRule || ruleType == Rule::GlobalAfterAppsRule) {
        if (confRuleManager()->rulesCountByType(ruleType) >= ConfUtil::ruleSetMaxCount()) {
            windowManager()->showErrorBox(tr("Global rules count exceeded!"));
            m_comboRuleType->setFocus();
            return false;
        }
    }

    return true;
}

void RuleEditDialog::fillRule(Rule &rule) const
{
    rule.ruleType = Rule::RuleType(m_comboRuleType->currentIndex());

    rule.enabled = m_cbEnabled->isChecked();
    rule.blocked = !m_rbAllow->isChecked();
    rule.exclusive = m_cbExclusive->isChecked();

    rule.acceptZones = m_btZones->zones();
    rule.rejectZones = m_btZones->uncheckedZones();

    rule.ruleName = m_editName->text();
    rule.notes = m_editNotes->toPlainText();
    rule.ruleText = m_editRuleText->toPlainText();

    rule.ruleSetEdited = ruleSetModel()->edited();
    rule.ruleSet = ruleSetModel()->ruleSet();
}

void RuleEditDialog::selectPresetRuleDialog()
{
    auto rulesDialog = RulesWindow::showRulesDialog(Rule::PresetRule, this);

    connect(rulesDialog, &RulesWindow::ruleSelected, ruleSetModel(), &RuleSetModel::addRule);
}
