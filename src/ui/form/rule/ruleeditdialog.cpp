#include "ruleeditdialog.h"

#include <QAction>
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
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/rulesetmodel.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/osutil.h>

#include "rulescontroller.h"
#include "ruleswindow.h"

using namespace Fort;

namespace {

const QSize appIconSize(32, 32);

}

RuleEditDialog::RuleEditDialog(RulesController *ctrl, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl), m_ruleSetModel(new RuleSetModel(this))
{
    setupUi();
    setupController();
}

void RuleEditDialog::initialize(const RuleRow &ruleRow)
{
    m_ruleRow = ruleRow;

    initializeRuleSet();

    retranslateUi();

    m_editName->setStartText(ruleRow.ruleName);
    m_editName->setClearButtonEnabled(true);

    const auto ruleIconPath = RuleListModel::ruleTypeIconPaths().value(ruleRow.ruleType);
    m_labelEditNotes->setPixmap(IconCache::pixmap(ruleIconPath, appIconSize));
    m_editNotes->setText(ruleRow.notes);

    m_labelRuleType->setText(tr("Type:"));
    m_comboRuleType->setCurrentIndex(ruleRow.ruleType);
    m_comboRuleType->setEnabled(isEmpty());

    m_cbEnabled->setChecked(ruleRow.enabled);
    m_cbTrayMenu->setChecked(ruleRow.trayMenu);

    m_rbAllow->setChecked(!ruleRow.blocked);
    m_rbBlock->setChecked(ruleRow.blocked);

    m_editRuleText->setText(ruleRow.ruleText);

    m_cbExclusive->setChecked(ruleRow.exclusive);

    m_btZones->setZones(ruleRow.zones.accept_mask);
    m_btZones->setUncheckedZones(ruleRow.zones.reject_mask);
    updateZonesLayout();

    m_cbInlineZones->setChecked(ruleRow.inlineZones);

    m_cbTerminate->setChecked(ruleRow.terminate);
    m_comboTerminateAction->setCurrentIndex(ruleRow.terminateActionType());
    m_cbTerminateAlert->setChecked(ruleRow.terminateAlert);

    m_cbLogAllowedConn->setChecked(ruleRow.logAllowedConn);
    m_cbLogBlockedConn->setChecked(ruleRow.logBlockedConn);

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
    m_cbTrayMenu->setText(tr("Tray Menu"));

    m_rbAllow->setText(tr("Allow"));
    m_rbBlock->setText(tr("Block"));

    m_cbExclusive->setText(tr("Exclusive"));
    m_btZones->retranslateUi();
    m_cbInlineZones->setText(tr("Inline Zones"));

    retranslateRulePlaceholderText();
    m_actRuleHelp->setText(tr("Help"));

    m_btAddPresetRule->setText(tr("Add Preset Rule"));
    m_btRemovePresetRule->setText(tr("Remove"));
    m_btEditPresetRule->setText(tr("Edit"));
    m_btUpPresetRule->setToolTip(tr("Move Up"));
    m_btDownPresetRule->setToolTip(tr("Move Down"));

    m_cbTerminate->setText(tr("Terminating Rule:"));
    retranslateComboTerminate();
    m_cbTerminateAlert->setText(tr("Alert"));

    m_cbLogAllowedConn->setText(tr("Collect allowed connections"));
    m_cbLogBlockedConn->setText(tr("Collect blocked connections"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Rule"));
}

void RuleEditDialog::retranslateComboRuleType()
{
    ControlUtil::setComboBoxTexts(m_comboRuleType, RuleListModel::ruleTypeNames());
    ControlUtil::setComboBoxIcons(m_comboRuleType, RuleListModel::ruleTypeIconPaths());
}

void RuleEditDialog::retranslateRulePlaceholderText()
{
    const auto placeholderText = tr("# Examples:")
            // IP-Address:Port
            + '\n' + tr("# IP address and port:")
            + "\n1.1.1.1:udp(53)"
              "\n(1.1.1.1-8.8.8.8):(53,80-8080)"
              "\n1.1.1.1:80:dir(in)";

    m_editRuleText->setPlaceholderText(placeholderText);
}

void RuleEditDialog::retranslateComboTerminate()
{
    const QStringList list = { tr("Allow"), tr("Block") };

    ControlUtil::setComboBoxTexts(m_comboTerminateAction, list);

    ControlUtil::setComboBoxIcons(
            m_comboTerminateAction, { ":/icons/accept.png", ":/icons/deny.png" });
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

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/script.png"));

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
    setupEditRuleText();

    // RuleSet Header
    auto ruleSetHeaderLayout = setupRuleSetHeaderLayout();

    // RuleSet View
    setupRuleSetView();

    // Actions on rule set view's current changed
    setupRuleSetViewChanged();

    // Terminate Layout
    auto terminateLayout = setupTerminateLayout();

    // Log Layout
    auto logLayout = setupLogLayout();

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
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(terminateLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(logLayout);
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
    m_labelEditNotes->setFixedSize(appIconSize);

    // Rule Type
    m_comboRuleType = ControlUtil::createComboBox();
    m_comboRuleType->setMinimumWidth(100);

    layout->addRow("Type:", m_comboRuleType);
    m_labelRuleType = ControlUtil::formRowLabel(layout, m_comboRuleType);

    // Enabled; Tray Menu
    auto enabledLayout = setupEnabledLayout();

    layout->addRow(QString(), enabledLayout);

    return layout;
}

QLayout *RuleEditDialog::setupEnabledLayout()
{
    m_cbEnabled = new QCheckBox();
    m_cbTrayMenu = new QCheckBox();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbEnabled, ControlUtil::createVSeparator(), m_cbTrayMenu, /*stretch*/ nullptr });
    layout->setSpacing(10);

    return layout;
}

QLayout *RuleEditDialog::setupActionsLayout()
{
    // Allow
    m_rbAllow = new QRadioButton();
    m_rbAllow->setIcon(IconCache::icon(":/icons/accept.png"));
    m_rbAllow->setChecked(true);

    connect(m_rbAllow, &QRadioButton::toggled, this, &RuleEditDialog::updateZonesLayout);

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

    // Inline Zones
    m_cbInlineZones = new QCheckBox();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbExclusive,
            ControlUtil::createVSeparator(), m_btZones, m_cbInlineZones, /*stretch*/ nullptr });

    return layout;
}

void RuleEditDialog::setupEditRuleText()
{
    // Help Action
    m_actRuleHelp = new QAction(IconCache::icon(":/icons/help.png"), QString(), this);

    connect(m_actRuleHelp, &QAction::triggered, this,
            [&] { OsUtil::openUrl(QUrl("https://github.com/tnodir/fort/wiki/Rules")); });

    // Edit Rule Text
    m_editRuleText = new PlainTextEdit();

    m_editRuleText->addContextAction(m_actRuleHelp);
}

QLayout *RuleEditDialog::setupRuleSetHeaderLayout()
{
    m_btAddPresetRule =
            ControlUtil::createFlatToolButton(":/icons/add.png", [&] { selectPresetRuleDialog(); });
    m_btRemovePresetRule = ControlUtil::createFlatToolButton(
            ":/icons/delete.png", [&] { ruleSetModel()->remove(ruleSetCurrentIndex()); });
    m_btEditPresetRule = ControlUtil::createFlatToolButton(
            ":/icons/pencil.png", [&] { editCurrentPresetRuleDialog(); });
    m_btUpPresetRule = ControlUtil::createIconToolButton(
            ":/icons/bullet_arrow_up.png", [&] { ruleSetModel()->moveUp(ruleSetCurrentIndex()); });
    m_btDownPresetRule = ControlUtil::createIconToolButton(":/icons/bullet_arrow_down.png",
            [&] { ruleSetModel()->moveDown(ruleSetCurrentIndex()); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btAddPresetRule, m_btRemovePresetRule,
            m_btEditPresetRule, ControlUtil::createVSeparator(), m_btUpPresetRule,
            m_btDownPresetRule, /*stretch*/ nullptr });

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

    connect(m_ruleSetView, &ListView::doubleClicked, m_btEditPresetRule, &QToolButton::click);

    connect(ruleSetModel(), &RuleSetModel::rowCountChanged, this,
            &RuleEditDialog::updateRuleSetViewVisible);
}

void RuleEditDialog::setupRuleSetViewChanged()
{
    const auto refreshRuleSetViewChanged = [&] {
        const bool ruleSelected = (ruleSetCurrentIndex() >= 0);
        m_btRemovePresetRule->setEnabled(ruleSelected);
        m_btEditPresetRule->setEnabled(ruleSelected);
        m_btUpPresetRule->setEnabled(ruleSelected);
        m_btDownPresetRule->setEnabled(ruleSelected);
    };

    refreshRuleSetViewChanged();

    connect(m_ruleSetView, &ListView::currentIndexChanged, this, refreshRuleSetViewChanged);
}

QLayout *RuleEditDialog::setupTerminateLayout()
{
    // Terminate Action
    m_comboTerminateAction = ControlUtil::createComboBox();
    m_comboTerminateAction->setMinimumWidth(100);

    // Terminate Alert
    m_cbTerminateAlert = new QCheckBox();

    // Terminate Check Box
    setupCbTerminate();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbTerminate, m_comboTerminateAction, m_cbTerminateAlert, /*stretch*/ nullptr });

    return layout;
}

void RuleEditDialog::setupCbTerminate()
{
    m_cbTerminate = new QCheckBox();

    const auto refreshTerminateEnabled = [&](bool checked) {
        m_comboTerminateAction->setEnabled(checked);
        m_cbTerminateAlert->setEnabled(checked);
    };

    refreshTerminateEnabled(false);

    connect(m_cbTerminate, &QCheckBox::toggled, this, refreshTerminateEnabled);
}

QLayout *RuleEditDialog::setupLogLayout()
{
    m_cbLogAllowedConn = new QCheckBox();
    m_cbLogBlockedConn = new QCheckBox();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbLogAllowedConn,
            ControlUtil::createVSeparator(), m_cbLogBlockedConn, /*stretch*/ nullptr });

    return layout;
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

void RuleEditDialog::updateZonesLayout()
{
    const bool enabled = m_rbAllow->isChecked();

    m_cbExclusive->setEnabled(enabled);
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

    // Add new rule
    if (isEmpty()) {
        return ctrl()->addOrUpdateRule(rule);
    }

    // Edit selected rule
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
    if (!validateEditName())
        return false;

    // Global Rules Count
    if (!validateComboRuleType())
        return false;

    // Rule Text
    if (!validateEditRuleText())
        return false;

    return true;
}

bool RuleEditDialog::validateEditName() const
{
    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    return true;
}

bool RuleEditDialog::validateComboRuleType() const
{
    const auto ruleType = Rule::RuleType(m_comboRuleType->currentIndex());
    if (ruleType == Rule::GlobalBeforeAppsRule || ruleType == Rule::GlobalAfterAppsRule) {
        if (confRuleManager()->rulesCountByType(ruleType) >= ConfUtil::ruleGlobalMaxCount()) {
            windowManager()->showErrorBox(tr("Global rules count exceeded!"));
            m_comboRuleType->setFocus();
            return false;
        }
    }

    return true;
}

bool RuleEditDialog::validateEditRuleText() const
{
    const auto ruleText = m_editRuleText->toPlainText();
    if (!ruleText.isEmpty()) {
        ConfBuffer confBuf;
        if (!confBuf.validateRuleText(ruleText)) {
            windowManager()->showErrorBox(confBuf.errorMessage());
            m_editRuleText->setFocus();
            return false;
        }
    }

    return true;
}

void RuleEditDialog::fillRule(Rule &rule) const
{
    rule.ruleType = Rule::RuleType(m_comboRuleType->currentIndex());

    rule.enabled = m_cbEnabled->isChecked();
    rule.trayMenu = m_cbTrayMenu->isChecked();
    rule.blocked = !m_rbAllow->isChecked();
    rule.exclusive = m_cbExclusive->isChecked();

    rule.terminate = m_cbTerminate->isChecked();
    rule.setTerminateActionType(m_comboTerminateAction->currentIndex());
    rule.terminateAlert = m_cbTerminateAlert->isChecked();

    rule.logAllowedConn = m_cbLogAllowedConn->isChecked();
    rule.logBlockedConn = m_cbLogBlockedConn->isChecked();

    rule.zones.accept_mask = m_btZones->zones();
    rule.zones.reject_mask = m_btZones->uncheckedZones();

    rule.inlineZones = m_cbInlineZones->isChecked();

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

void RuleEditDialog::editCurrentPresetRuleDialog()
{
    const int ruleId = ruleSetModel()->ruleSet().value(ruleSetCurrentIndex());

    RulesWindow::showRuleEditDialog(ruleId, Rule::PresetRule, this);
}
