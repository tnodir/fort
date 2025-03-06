#include "programeditdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/spincombo.h>
#include <form/controls/zonesselector.h>
#include <form/dialog/dialogutil.h>
#include <form/rule/ruleswindow.h>
#include <fortmanager.h>
#include <manager/windowmanager.h>
#include <model/rulelistmodel.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/stringutil.h>
#include <util/textareautil.h>
#include <util/variantutil.h>

#include "programscontroller.h"

namespace {

const QSize appIconSize(32, 32);

const std::array appBlockInMinuteValues = { 15, 0, 1, 5, 10, 30, 60 * 1, 60 * 6, 60 * 12, 60 * 24,
    60 * 24 * 7, 60 * 24 * 30 };

}

enum ScheduleType : qint8 {
    ScheduleTimeIn = 0,
    ScheduleTimeAt,
};

ProgramEditDialog::ProgramEditDialog(ProgramsController *ctrl, QWidget *parent, Qt::WindowFlags f) :
    FormWindow(parent, (f == Qt::Widget ? Qt::Dialog : f)), m_ctrl(ctrl)
{
    setupUi();
    setupController();
    setupRuleManager();
}

FortManager *ProgramEditDialog::fortManager() const
{
    return ctrl()->fortManager();
}

ConfAppManager *ProgramEditDialog::confAppManager() const
{
    return ctrl()->confAppManager();
}

ConfRuleManager *ProgramEditDialog::confRuleManager() const
{
    return ctrl()->confRuleManager();
}

ConfManager *ProgramEditDialog::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *ProgramEditDialog::conf() const
{
    return ctrl()->conf();
}

IniUser *ProgramEditDialog::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *ProgramEditDialog::windowManager() const
{
    return ctrl()->windowManager();
}

AppListModel *ProgramEditDialog::appListModel() const
{
    return ctrl()->appListModel();
}

void ProgramEditDialog::initialize(const AppRow &appRow, const QVector<qint64> &appIdList)
{
    const bool isSingleSelection = (appIdList.size() <= 1);

    m_isWildcard = appRow.isWildcard;
    m_appRow = appRow;
    m_appIdList = appIdList;

    retranslateUi();

    initializePathNameRuleFields(isSingleSelection);

    updateApplyChild();
    m_comboAppGroup->setCurrentIndex(appRow.groupIndex);

    m_rbAllow->setChecked(!appRow.blocked);
    m_rbBlock->setChecked(appRow.blocked);
    m_rbKillProcess->setChecked(appRow.killProcess);

    m_cbKillChild->setChecked(appRow.killChild);

    m_cbParked->setChecked(appRow.parked);
    m_cbLogAllowedConn->setChecked(appRow.logAllowedConn);
    m_cbLogBlockedConn->setChecked(appRow.logBlockedConn);

    m_cbLanOnly->setChecked(appRow.lanOnly);
    m_btZones->setZones(appRow.zones.accept_mask);
    m_btZones->setUncheckedZones(appRow.zones.reject_mask);
    updateZonesRulesLayout();

    const bool hasScheduleTime = !appRow.scheduleTime.isNull();
    const ScheduleType scheduleType = hasScheduleTime ? ScheduleTimeAt : ScheduleTimeIn;

    m_cbSchedule->setChecked(hasScheduleTime);
    m_comboScheduleAction->setCurrentIndex(appRow.scheduleAction);
    m_comboScheduleType->setCurrentIndex(scheduleType);
    m_scScheduleIn->spinBox()->setValue(5);
    m_dteScheduleAt->setDateTime(appRow.scheduleTime);
    m_dteScheduleAt->setMinimumDateTime(DateUtil::now());

    m_btSwitchWildcard->setChecked(isWildcard());
    m_btSwitchWildcard->setEnabled(isSingleSelection);

    updateWildcard(isSingleSelection);

    initializeFocus();
}

void ProgramEditDialog::initializePathNameRuleFields(bool isSingleSelection)
{
    initializePathField(isSingleSelection);
    initializeNameField(isSingleSelection);
    initializeRuleField(isSingleSelection);
}

void ProgramEditDialog::initializePathField(bool isSingleSelection)
{
    const bool isPathEditable = isSingleSelection && (isNew() || isWildcard());

    m_editPath->setReadOnly(!isPathEditable);
    m_editPath->setClearButtonEnabled(isPathEditable);

    m_editPath->setText(isSingleSelection && !isWildcard() ? m_appRow.appOriginPath : QString());
    m_editPath->setEnabled(isSingleSelection);

    m_editWildcard->setText(isSingleSelection && isWildcard() ? m_appRow.appOriginPath : QString());
    m_editWildcard->setEnabled(isSingleSelection);

    m_btSelectFile->setEnabled(isSingleSelection);
}

void ProgramEditDialog::initializeNameField(bool isSingleSelection)
{
    m_editName->setStartText(isSingleSelection ? m_appRow.appName : QString());
    m_editName->setEnabled(isSingleSelection);
    m_editName->setClearButtonEnabled(isSingleSelection);

    m_btGetName->setEnabled(isSingleSelection);

    m_editNotes->setText(m_appRow.notes);
    m_editNotes->setEnabled(isSingleSelection);

    if (isSingleSelection) {
        if (m_appRow.appName.isEmpty()) {
            fillEditName(); // Auto-fill the name
        }
    }
}

void ProgramEditDialog::initializeRuleField(bool isSingleSelection)
{
    setCurrentRuleId(m_appRow.ruleId);

    m_editRuleName->setStartText(isSingleSelection ? m_appRow.ruleName : QString());
    m_editRuleName->setEnabled(isSingleSelection);
    m_editRuleName->setClearButtonEnabled(isSingleSelection);

    m_btSelectRule->setEnabled(isSingleSelection);
}

void ProgramEditDialog::initializeFocus()
{
    if (!isNew()) {
        m_btgActions->checkedButton()->setFocus();
    } else if (isWildcard()) {
        m_editWildcard->setFocus();
    } else {
        m_editPath->setFocus();
    }
}

QIcon ProgramEditDialog::appIcon(bool isSingleSelection) const
{
    if (!isSingleSelection)
        return {};

    if (isWildcard()) {
        return IconCache::icon(":/icons/coding.png");
    }

    return IoC<AppInfoCache>()->appIcon(m_appRow.appPath);
}

void ProgramEditDialog::closeOnSave()
{
    this->close();
}

void ProgramEditDialog::setAdvancedMode(bool on)
{
    m_rbKillProcess->setVisible(on);
}

void ProgramEditDialog::setupController()
{
    connect(ctrl(), &ProgramsController::retranslateUi, this, &ProgramEditDialog::retranslateUi);
}

void ProgramEditDialog::setupRuleManager()
{
    connect(confRuleManager(), &ConfRuleManager::ruleRemoved, this,
            [&](quint16 ruleId, int appRulesCount) {
                if (appRulesCount > 0 && ruleId == currentRuleId()) {
                    setCurrentRuleId();
                    m_editRuleName->clear();
                }
            });

    connect(confRuleManager(), &ConfRuleManager::ruleUpdated, this, [&](quint16 ruleId) {
        if (ruleId == currentRuleId()) {
            const auto ruleRow = RuleListModel().ruleRowById(ruleId, Rule::AppRule);

            m_editRuleName->setStartText(ruleRow.ruleName);
        }
    });
}

void ProgramEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelEditPath->setText(isWildcard() ? tr("Wildcard Paths:") : tr("File Path:"));
    retranslatePathPlaceholderText();
    m_btSelectFile->setToolTip(tr("Select File"));

    m_labelEditName->setText(tr("Name:"));
    m_btGetName->setToolTip(tr("Get Program Name"));

    m_editNotes->setPlaceholderText(tr("Notes"));

    m_cbApplyChild->setText(tr("Rules inheritance:"));
    retranslateComboApplyChild();

    m_labelAppGroup->setText(tr("Group:"));

    m_rbAllow->setText(tr("Allow"));
    m_rbBlock->setText(tr("Block"));
    m_rbKillProcess->setText(tr("Kill Process"));

    m_cbKillChild->setText(tr("Kill child processes"));

    m_cbParked->setText(tr("Parked"));
    m_cbParked->setToolTip(tr("Don't purge as obsolete"));
    m_cbLogAllowedConn->setText(tr("Collect allowed connections"));
    m_cbLogBlockedConn->setText(tr("Collect blocked connections"));

    m_cbLanOnly->setText(tr("Block Internet Traffic"));
    m_btZones->retranslateUi();

    m_editRuleName->setPlaceholderText(tr("Rule"));
    m_btSelectRule->setToolTip(tr("Select Rule"));

    m_cbSchedule->setText(tr("Schedule"));
    retranslateScheduleAction();
    retranslateScheduleType();
    retranslateScheduleIn();
    m_dteScheduleAt->unsetLocale();

    m_btOptions->setToolTip(tr("Options"));
    m_btSwitchWildcard->setToolTip(tr("Switch Wildcard"));
    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    retranslateWindowTitle();
}

void ProgramEditDialog::retranslatePathPlaceholderText()
{
    if (!(isWildcard() && m_editWildcard->isEnabled()))
        return;

    const auto placeholderText = tr("# Examples:")
            // Prefix wildcard
            + '\n' + tr("# All programs in the sub-path:")
            + "\nC:\\Git\\**"
            // Name wildcard
            + '\n' + tr("# Name wildcard:")
            + "\nC:\\Store\\app.v*.exe"
              "\nC:\\App.v*\\app.exe"
            // Env var
            + '\n' + tr("# Environment Variable:")
            + "\n%SystemRoot%\\System32\\telnet.exe"
              "\n%ProgramFiles%\\Internet Explorer\\iexplore.exe";

    m_editWildcard->setPlaceholderText(placeholderText);
}

void ProgramEditDialog::retranslateComboApplyChild()
{
    // Sync with ProgramEditDialog::ApplyChildType
    const QStringList list = { tr("Propagate to all child processes"),
        tr("Propagate to designated child processes"), tr("Receive from the parent process") };

    ControlUtil::setComboBoxTexts(m_comboApplyChild, list);

    updateApplyChild();
}

void ProgramEditDialog::retranslateScheduleAction()
{
    const QStringList list = { tr("Block"), tr("Allow"), tr("Remove"), tr("Kill Process") };

    ControlUtil::setComboBoxTexts(m_comboScheduleAction, list);

    ControlUtil::setComboBoxIcons(m_comboScheduleAction,
            { ":/icons/deny.png", ":/icons/accept.png", ":/icons/delete.png",
                    ":/icons/scull.png" });
}

void ProgramEditDialog::retranslateScheduleType()
{
    const QStringList list = { tr("In:"), tr("At:") };

    ControlUtil::setComboBoxTexts(m_comboScheduleType, list);
}

void ProgramEditDialog::retranslateScheduleIn()
{
    const QStringList list = { tr("Custom"), tr("1 second"), tr("1 minute"), tr("5 minutes"),
        tr("10 minutes"), tr("30 minutes"), tr("1 hour"), tr("6 hours"), tr("12 hours"), tr("Day"),
        tr("Week"), tr("Month") };

    m_scScheduleIn->setNames(list);
    m_scScheduleIn->spinBox()->setSuffix(tr(" minute(s)"));
}

void ProgramEditDialog::retranslateWindowTitle()
{
    this->setWindowTitle(isWildcard() ? tr("Edit Wildcard") : tr("Edit Program"));
}

void ProgramEditDialog::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Advanced Mode
    setAdvancedMode(true);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/application.png"));

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Size
    this->setMinimumWidth(500);
}

QLayout *ProgramEditDialog::setupMainLayout()
{
    // Form Layout
    auto formLayout = setupFormLayout();

    // Apply Child, Group Layout
    auto applyChildGroupLayout = setupApplyChildGroupLayout();

    // Allow/Block/Kill Actions Layout
    auto actionsLayout = setupActionsLayout();

    setupActionsGroup();

    // Zones/Rules
    auto zonesRulesLayout = setupZonesRuleLayout();

    // Schedule
    auto scheduleLayout = setupScheduleLayout();

    // Menu, OK/Cancel
    auto buttonsLayout = setupButtonsLayout();

    // Form
    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
    layout->addLayout(applyChildGroupLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addStretch();
    layout->addLayout(actionsLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(zonesRulesLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(scheduleLayout);
    layout->addStretch();
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *ProgramEditDialog::setupFormLayout()
{
    auto layout = new QFormLayout();
    layout->setHorizontalSpacing(10);

    // Path
    auto pathLayout = setupPathLayout();

    layout->addRow("File Path:", pathLayout);
    m_labelEditPath = ControlUtil::formRowLabel(layout, pathLayout);

    // Name
    auto nameLayout = setupNameLayout();

    layout->addRow("Name:", nameLayout);
    m_labelEditName = ControlUtil::formRowLabel(layout, nameLayout);

    // Notes
    m_editNotes = new PlainTextEdit();
    m_editNotes->setFixedHeight(40);

    layout->addRow("Notes:", m_editNotes);
    m_labelEditNotes = ControlUtil::formRowLabel(layout, m_editNotes);
    m_labelEditNotes->setScaledContents(true);
    m_labelEditNotes->setFixedSize(appIconSize);

    return layout;
}

QLayout *ProgramEditDialog::setupPathLayout()
{
    // Path
    m_editPath = new LineEdit();
    m_editPath->setMaxLength(1024);

    // Wildcard
    m_editWildcard = new PlainTextEdit();

    // Select File
    m_btSelectFile = ControlUtil::createIconToolButton(":/icons/folder.png", [&] {
        if (!isWildcard() && m_editPath->isReadOnly()) {
            AppInfoUtil::openFolder(m_editPath->text());
            return;
        }

        const auto filePath = DialogUtil::getOpenFileName(
                m_labelEditPath->text(), tr("Programs (*.exe);;All files (*.*)"));

        if (filePath.isEmpty())
            return;

        const auto appPath = FileUtil::toNativeSeparators(filePath);

        if (isWildcard()) {
            TextAreaUtil::appendText(m_editWildcard, appPath);
        } else {
            m_editPath->setText(appPath);
        }

        fillEditName(); // Auto-fill the name
    });
    m_btSelectFile->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_O));

    auto layout = new QHBoxLayout();
    layout->addWidget(m_editPath);
    layout->addWidget(m_editWildcard);
    layout->addWidget(m_btSelectFile, 0, Qt::AlignTop);

    return layout;
}

QLayout *ProgramEditDialog::setupNameLayout()
{
    m_editName = new LineEdit();
    m_editName->setMaxLength(1024);

    m_btGetName = ControlUtil::createIconToolButton(
            ":/icons/arrow_refresh_small.png", [&] { fillEditName(); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_editName, m_btGetName });

    return layout;
}

QLayout *ProgramEditDialog::setupApplyChildGroupLayout()
{
    // Apply Child
    m_comboApplyChild =
            ControlUtil::createComboBox({}, [&](int /*index*/) { warnRestartNeededOption(); });
    m_comboApplyChild->setFixedWidth(150);

    setupCbApplyChild();

    // Group
    setupComboAppGroups();

    m_labelAppGroup = ControlUtil::createLabel();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbApplyChild,
            /*stretch*/ nullptr, m_comboApplyChild, ControlUtil::createVSeparator(),
            m_labelAppGroup, /*stretch*/ nullptr, m_comboAppGroup });

    return layout;
}

void ProgramEditDialog::setupCbApplyChild()
{
    m_cbApplyChild = ControlUtil::createCheckBox(":/icons/document_tree.png");

    const auto refreshApplyChildEnabled = [&](bool checked) {
        m_comboApplyChild->setEnabled(checked);
    };

    refreshApplyChildEnabled(false);

    connect(m_cbApplyChild, &QCheckBox::toggled, this, refreshApplyChildEnabled);
}

void ProgramEditDialog::setupComboAppGroups()
{
    m_comboAppGroup = ControlUtil::createComboBox();
    m_comboAppGroup->setFixedWidth(150);

    const auto refreshComboAppGroups = [&](bool onlyFlags = false) {
        if (onlyFlags)
            return;

        ControlUtil::setComboBoxTexts(m_comboAppGroup, conf()->appGroupNames(), /*currentIndex=*/0);
    };

    refreshComboAppGroups();

    connect(confManager(), &ConfManager::confChanged, this, refreshComboAppGroups);
}

QLayout *ProgramEditDialog::setupActionsLayout()
{
    // Allow
    m_rbAllow = new QRadioButton();
    m_rbAllow->setIcon(IconCache::icon(":/icons/accept.png"));
    m_rbAllow->setChecked(true);

    // Block
    m_rbBlock = new QRadioButton();
    m_rbBlock->setIcon(IconCache::icon(":/icons/deny.png"));

    // Kill Process
    m_rbKillProcess = new QRadioButton();
    m_rbKillProcess->setIcon(IconCache::icon(":/icons/scull.png"));

    connect(m_rbKillProcess, &QRadioButton::clicked, this, &ProgramEditDialog::warnDangerousOption);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { /*stretch*/ nullptr, m_rbAllow, m_rbBlock, m_rbKillProcess, /*stretch*/ nullptr });
    layout->setSpacing(20);

    return layout;
}

void ProgramEditDialog::setupActionsGroup()
{
    m_btgActions = new QButtonGroup(this);
    m_btgActions->setExclusive(true);

    m_btgActions->addButton(m_rbAllow);
    m_btgActions->addButton(m_rbBlock);
    m_btgActions->addButton(m_rbKillProcess);

    connect(m_rbAllow, &QRadioButton::toggled, this, &ProgramEditDialog::updateZonesRulesLayout);
}

QLayout *ProgramEditDialog::setupZonesRuleLayout()
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

QLayout *ProgramEditDialog::setupRuleLayout()
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

QLayout *ProgramEditDialog::setupScheduleLayout()
{
    // Schedule Action
    m_comboScheduleAction = ControlUtil::createComboBox();
    m_comboScheduleAction->setMinimumWidth(100);

    // Schedule Type
    setupComboScheduleType();

    // Schedule after N minutes
    m_scScheduleIn = new SpinCombo();
    m_scScheduleIn->spinBox()->setRange(0, 60 * 24 * 30 * 12); // ~Year
    m_scScheduleIn->setValues(appBlockInMinuteValues);

    // Schedule to a specified date & time
    m_dteScheduleAt = new QDateTimeEdit();
    m_dteScheduleAt->setCalendarPopup(true);

    // Schedule Check Box
    setupCbSchedule();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbSchedule, m_comboScheduleAction,
            m_comboScheduleType, /*stretch*/ nullptr, m_scScheduleIn, m_dteScheduleAt });

    return layout;
}

void ProgramEditDialog::setupCbSchedule()
{
    m_cbSchedule = ControlUtil::createCheckBox(":/icons/time.png");

    const auto refreshScheduleEnabled = [&](bool checked) {
        m_comboScheduleAction->setEnabled(checked);
        m_comboScheduleType->setEnabled(checked);
        m_scScheduleIn->setEnabled(checked);
        m_dteScheduleAt->setEnabled(checked);
    };

    refreshScheduleEnabled(false);

    connect(m_cbSchedule, &QCheckBox::toggled, this, refreshScheduleEnabled);
}

void ProgramEditDialog::setupComboScheduleType()
{
    m_comboScheduleType = ControlUtil::createComboBox();

    connect(m_comboScheduleType, &QComboBox::currentIndexChanged, this, [&](int index) {
        m_scScheduleIn->setVisible(index == ScheduleTimeIn);
        m_dteScheduleAt->setVisible(index == ScheduleTimeAt);
    });
}

QLayout *ProgramEditDialog::setupButtonsLayout()
{
    // Options
    setupOptions();

    // Switch Wildcard
    setupSwitchWildcard();

    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] {
        if (save()) {
            closeOnSave();
        }
    });
    m_btOk->setDefault(true);

    connect(this, &WidgetWindow::defaultKeyPressed, m_btOk, &QAbstractButton::click);

    // Cancel
    m_btCancel = new QPushButton();
    connect(m_btCancel, &QAbstractButton::clicked, this, &QWidget::close);

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btSwitchWildcard, m_btOptions,
            /*stretch*/ nullptr, m_btOk, m_btCancel, m_btMenu });

    return layout;
}

void ProgramEditDialog::setupOptions()
{
    // Parked
    m_cbParked = ControlUtil::createCheckBox(":/icons/parking.png");

    // Child Options
    setupChildOptionsLayout();

    // Log Options
    setupLogOptions();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbKillChild);
    layout->addWidget(m_cbParked);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_cbLogAllowedConn);
    layout->addWidget(m_cbLogBlockedConn);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btOptions = ControlUtil::createButton(":/icons/widgets.png");
    m_btOptions->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_M));
    m_btOptions->setMenu(menu);
}

void ProgramEditDialog::setupChildOptionsLayout()
{
    // Kill Child
    m_cbKillChild = ControlUtil::createCheckBox(":/icons/scull.png");

    connect(m_cbKillChild, &QCheckBox::clicked, this, &ProgramEditDialog::warnDangerousOption);
}

void ProgramEditDialog::setupLogOptions()
{
    // Log Allowed Connections
    m_cbLogAllowedConn = new QCheckBox();

    // Log Blocked Connections
    m_cbLogBlockedConn = new QCheckBox();
}

void ProgramEditDialog::setupSwitchWildcard()
{
    m_btSwitchWildcard = ControlUtil::createIconToolButton(":/icons/coding.png", [&] {
        m_isWildcard = !m_isWildcard;

        switchWildcardPaths();

        updateWildcard();
        retranslateUi();
    });

    m_btSwitchWildcard->setCheckable(true);
}

void ProgramEditDialog::updateZonesRulesLayout()
{
    const bool enabled = m_rbAllow->isChecked();

    m_cbLanOnly->setEnabled(enabled);
    m_btZones->setEnabled(enabled);

    const bool isSingleSelection = (m_appIdList.size() <= 1);
    if (!isSingleSelection)
        return;

    m_btSelectRule->setEnabled(enabled);
}

void ProgramEditDialog::updateApplyChild()
{
    const ApplyChildType type = m_appRow.applyParent ? ApplyChildType::FromParent
            : m_appRow.applySpecChild                ? ApplyChildType::ToSpecChild
            : m_appRow.applyChild                    ? ApplyChildType::ToChild
                                                     : ApplyChildType::Invalid;
    const bool hasApplyChild = (type != ApplyChildType::Invalid);

    m_cbApplyChild->setChecked(hasApplyChild);
    m_comboApplyChild->setCurrentIndex(int(hasApplyChild ? type : ApplyChildType::ToChild));
}

void ProgramEditDialog::updateWildcard(bool isSingleSelection)
{
    m_editPath->setVisible(!isWildcard());

    m_editWildcard->setVisible(isWildcard());

    m_labelEditNotes->setPixmap(appIcon(isSingleSelection).pixmap(appIconSize));
}

void ProgramEditDialog::switchWildcardPaths()
{
    if (isWildcard()) {
        m_editWildcard->setText(m_editPath->text());
        return;
    }

    if (!m_editPath->isReadOnly()) {
        const auto line = StringUtil::firstLine(m_editWildcard->toPlainText());

        m_editPath->setText(line);
    }
}

void ProgramEditDialog::fillEditName()
{
    const QString appPath = getEditText();
    if (appPath.isEmpty())
        return;

    QString appName;
    if (isWildcard()) {
        appName = StringUtil::firstLine(appPath);
    } else {
        const QString normPath = FileUtil::normalizePath(appPath);
        appName = IoC<AppInfoCache>()->appName(normPath);
    }

    m_editName->setStartText(appName);
}

bool ProgramEditDialog::save()
{
    const int appIdsCount = m_appIdList.size();
    const bool isSingleSelection = (appIdsCount <= 1);

    if (isSingleSelection && !validateFields())
        return false;

    App app;
    fillApp(app);

    // Add new app or edit non-selected app
    if (appIdsCount == 0) {
        return ctrl()->addOrUpdateApp(app);
    }

    // Edit selected app
    if (isSingleSelection) {
        return saveApp(app);
    }

    // Edit selected apps
    return saveMulti(app);
}

bool ProgramEditDialog::saveApp(App &app)
{
    if (!app.isOptionsEqual(m_appRow)) {
        app.appId = m_appRow.appId;

        return ctrl()->updateApp(app);
    }

    if (!app.isNameEqual(m_appRow)) {
        return ctrl()->updateAppName(m_appRow.appId, app.appName);
    }

    return true;
}

bool ProgramEditDialog::saveMulti(App &app)
{
    for (qint64 appId : std::as_const(m_appIdList)) {
        const auto appRow = appListModel()->appRowById(appId);

        app.appId = appId;
        app.appOriginPath = appRow.appOriginPath;
        app.appPath = appRow.appPath;
        app.appName = appRow.appName;

        if (!ctrl()->updateApp(app))
            return false;
    }

    return true;
}

bool ProgramEditDialog::validateFields() const
{
    // Path
    const bool isPathEmpty =
            isWildcard() ? m_editWildcard->isEmpty() : m_editPath->text().isEmpty();
    if (isPathEmpty) {
        QWidget *c = isWildcard() ? static_cast<QWidget *>(m_editWildcard)
                                  : static_cast<QWidget *>(m_editPath);
        c->setFocus();
        return false;
    }

    // Name
    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    return true;
}

void ProgramEditDialog::fillApp(App &app) const
{
    app.isWildcard = isWildcard();
    app.killChild = m_cbKillChild->isChecked();
    app.lanOnly = m_cbLanOnly->isChecked();
    app.parked = m_cbParked->isChecked();
    app.logAllowedConn = m_cbLogAllowedConn->isChecked();
    app.logBlockedConn = m_cbLogBlockedConn->isChecked();
    app.blocked = !m_rbAllow->isChecked();
    app.killProcess = m_rbKillProcess->isChecked();
    app.groupIndex = m_comboAppGroup->currentIndex();
    app.ruleId = currentRuleId();
    app.appName = m_editName->text();
    app.notes = m_editNotes->toPlainText();

    app.zones.accept_mask = m_btZones->zones();
    app.zones.reject_mask = m_btZones->uncheckedZones();

    fillAppPath(app);
    fillAppApplyChild(app);
    fillAppEndTime(app);
}

void ProgramEditDialog::fillAppPath(App &app) const
{
    const QString appPath = getEditText();

    app.appOriginPath = appPath;
    app.appPath = FileUtil::normalizePath(StringUtil::firstLine(appPath));
}

void ProgramEditDialog::fillAppApplyChild(App &app) const
{
    app.applyParent = false;
    app.applyChild = false;
    app.applySpecChild = false;

    if (!m_cbApplyChild->isChecked())
        return;

    const auto type = ApplyChildType(m_comboApplyChild->currentIndex());

    switch (type) {
    case ApplyChildType::ToChild: {
        app.applyChild = true;
    } break;
    case ApplyChildType::ToSpecChild: {
        app.applySpecChild = true;
        app.applyChild = true;
    } break;
    case ApplyChildType::FromParent: {
        app.applyParent = true;
    } break;
    }
}

void ProgramEditDialog::fillAppEndTime(App &app) const
{
    app.scheduleTime = {};

    if (!m_cbSchedule->isChecked()) {
        app.scheduleAction = App::ScheduleBlock;
        return;
    }

    app.scheduleAction = m_comboScheduleAction->currentIndex();

    switch (m_comboScheduleType->currentIndex()) {
    case ScheduleTimeIn: {
        const int minutes = m_scScheduleIn->spinBox()->value();

        app.scheduleTime = DateUtil::now().addSecs(minutes * 60);
    } break;
    case ScheduleTimeAt: {
        app.scheduleTime = m_dteScheduleAt->dateTime();
    } break;
    }
}

QString ProgramEditDialog::getEditText() const
{
    return isWildcard() ? m_editWildcard->toPlainText() : m_editPath->text();
}

void ProgramEditDialog::selectRuleDialog()
{
    auto rulesDialog = RulesWindow::showRulesDialog(Rule::AppRule, this);

    connect(rulesDialog, &RulesWindow::ruleSelected, this, [&](const RuleRow &ruleRow) {
        setCurrentRuleId(ruleRow.ruleId);
        m_editRuleName->setStartText(ruleRow.ruleName);
    });
}

void ProgramEditDialog::editRuleDialog(int ruleId)
{
    RulesWindow::showRuleEditDialog(ruleId, Rule::AppRule, this);
}

void ProgramEditDialog::warnDangerousOption() const
{
    IoC<WindowManager>()->showErrorBox(
            tr("Attention: This option is very dangerous!!!\n\n"
               "Be careful when killing a system services or other important programs!\n"
               "It can cause a Windows malfunction or totally unusable."));
}

void ProgramEditDialog::warnRestartNeededOption() const
{
    IoC<WindowManager>()->showErrorBox(
            tr("Attention: This option only affects new processes!\n\n"
               "Please restart the running program to take effect of this option."));
}
