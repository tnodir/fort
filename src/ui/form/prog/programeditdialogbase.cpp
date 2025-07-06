#include "programeditdialogbase.h"

#include <QActionGroup>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/spincombo.h>
#include <form/controls/tableview.h>
#include <form/controls/toolbutton.h>
#include <form/controls/zonesselector.h>
#include <form/dialog/dialogutil.h>
#include <form/rule/ruleswindow.h>
#include <fortmanager.h>
#include <manager/windowmanager.h>
#include <model/appconnlistmodel.h>
#include <model/applistmodel.h>
#include <model/rulelistmodel.h>
#include <user/iniuser.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>
#include <util/stringutil.h>
#include <util/textareautil.h>
#include <util/variantutil.h>

#include "programeditcontroller.h"

namespace {

const QSize appIconSize(32, 32);

const std::array appBlockInMinuteValues = { 15, 0, 1, 5, 10, 30, 60 * 1, 60 * 3, 60 * 6, 60 * 12,
    60 * 24, 60 * 24 * 7, 60 * 24 * 30 };

}

ProgramEditDialogBase::ProgramEditDialogBase(
        ProgramEditController *ctrl, QWidget *parent, Qt::WindowFlags f) :
    FormWindow(parent, (f == Qt::Widget ? Qt::Dialog : f)), m_ctrl(ctrl)
{
    setupUi();
    setupController();
    setupRuleManager();
}

FortManager *ProgramEditDialogBase::fortManager() const
{
    return ctrl()->fortManager();
}

ConfAppManager *ProgramEditDialogBase::confAppManager() const
{
    return ctrl()->confAppManager();
}

ConfRuleManager *ProgramEditDialogBase::confRuleManager() const
{
    return ctrl()->confRuleManager();
}

ConfManager *ProgramEditDialogBase::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *ProgramEditDialogBase::conf() const
{
    return ctrl()->conf();
}

IniUser *ProgramEditDialogBase::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *ProgramEditDialogBase::windowManager() const
{
    return ctrl()->windowManager();
}

QIcon ProgramEditDialogBase::appIcon(bool isSingleSelection) const
{
    if (!isSingleSelection)
        return {};

    if (isWildcard()) {
        return IconCache::icon(":/icons/coding.png");
    }

    return IoC<AppInfoCache>()->appIcon(m_app.appPath);
}

void ProgramEditDialogBase::setupController()
{
    connect(ctrl(), &ProgramEditController::retranslateUi, this,
            &ProgramEditDialogBase::retranslateUi);
}

void ProgramEditDialogBase::setupRuleManager()
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

void ProgramEditDialogBase::retranslateUi()
{
    this->unsetLocale();

    m_labelEditPath->setText(isWildcard() ? tr("Wildcard Paths:") : tr("File Path:"));
    retranslatePathPlaceholderText();
    m_btSelectFile->setToolTip(tr("Select File"));

    m_labelEditName->setText(tr("Name:"));
    m_btGetName->setToolTip(tr("Get Program Name"));

    m_editNotes->setPlaceholderText(tr("Notes"));
    m_btSetIcon->setToolTip(tr("Set Icon"));
    m_btDeleteIcon->setToolTip(tr("Delete Icon"));

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

    updateQuickAction();
    retranslateTimedMenuActions();
    retranslateTimedAction(m_btTimedAction);
    retranslateTimedAction(m_btTimedRemove);

    m_btSwitchWildcard->setToolTip(tr("Switch Wildcard"));
    m_btOptions->setToolTip(tr("Options"));
    m_btConnections->setToolTip(tr("Connections"));
    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    retranslateWindowTitle();
}

void ProgramEditDialogBase::retranslatePathPlaceholderText()
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

void ProgramEditDialogBase::retranslateComboApplyChild()
{
    // Sync with ProgramEditDialogBase::ApplyChildType
    const QStringList list = { tr("Propagate to all child processes"),
        tr("Propagate to designated child processes"), tr("Receive from the parent process") };

    ControlUtil::setComboBoxTexts(m_comboApplyChild, list);

    updateApplyChild();
}

void ProgramEditDialogBase::retranslateScheduleAction()
{
    const QStringList list = { tr("Block"), tr("Allow"), tr("Remove"), tr("Kill Process") };

    ControlUtil::setComboBoxTexts(m_comboScheduleAction, list);

    ControlUtil::setComboBoxIcons(m_comboScheduleAction,
            { ":/icons/deny.png", ":/icons/accept.png", ":/icons/delete.png",
                    ":/icons/scull.png" });
}

void ProgramEditDialogBase::retranslateScheduleType()
{
    const QStringList list = { tr("In:"), tr("At:") };

    ControlUtil::setComboBoxTexts(m_comboScheduleType, list);
}

void ProgramEditDialogBase::retranslateScheduleIn()
{
    const QStringList list = { tr("Custom"), tr("1 second"), tr("1 minute"), tr("5 minutes"),
        tr("10 minutes"), tr("30 minutes"), tr("1 hour"), tr("3 hours"), tr("6 hours"),
        tr("12 hours"), tr("Day"), tr("Week"), tr("Month") };

    m_scScheduleIn->setNames(list);
    m_scScheduleIn->spinBox()->setSuffix(tr(" minute(s)"));
}

void ProgramEditDialogBase::retranslateTimedMenuActions()
{
    const auto actions = m_timedMenuActions->actions();
    if (actions.isEmpty())
        return;

    const auto names = m_scScheduleIn->names();

    int index = 0;
    for (auto a : actions) {
        const auto name = names[index++];
        a->setText(name);
    }
}

void ProgramEditDialogBase::retranslateTimedAction(QToolButton *bt)
{
    const int minutes = timedActionMinutes(bt);

    const int index = m_scScheduleIn->getIndexByValue(minutes);
    const QString text =
            (index > 0) ? m_scScheduleIn->names().at(index) : tr("%1 minute(s)").arg(minutes);

    bt->setText(text);
}

void ProgramEditDialogBase::retranslateTableConnListMenu()
{
    m_actCopyAsFilter->setText(tr("Copy as Filter"));
    m_actCopy->setText(tr("Copy"));
    m_actLookupIp->setText(tr("Lookup IP"));
}

void ProgramEditDialogBase::retranslateWindowTitle()
{
    this->setWindowTitle(isWildcard() ? tr("Edit Wildcard") : tr("Edit Program"));
}

void ProgramEditDialogBase::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);
}

QLayout *ProgramEditDialogBase::setupMainLayout()
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
    layout->addLayout(actionsLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(zonesRulesLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addStretch();
    layout->addLayout(scheduleLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *ProgramEditDialogBase::setupFormLayout()
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
    auto notesLayout = setupNotesLayout();

    layout->addRow("Notes:", notesLayout);
    m_labelEditNotes = ControlUtil::formRowLabel(layout, notesLayout);
    m_labelEditNotes->setScaledContents(true);
    m_labelEditNotes->setFixedSize(appIconSize);

    return layout;
}

QLayout *ProgramEditDialogBase::setupPathLayout()
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

QLayout *ProgramEditDialogBase::setupNameLayout()
{
    m_editName = new LineEdit();
    m_editName->setMaxLength(1024);

    m_btGetName = ControlUtil::createIconToolButton(
            ":/icons/arrow_refresh_small.png", [&] { fillEditName(); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_editName, m_btGetName });

    return layout;
}

QLayout *ProgramEditDialogBase::setupNotesLayout()
{
    m_editNotes = new PlainTextEdit();
    m_editNotes->setFixedHeight(40);

    m_btSetIcon = ControlUtil::createIconToolButton(":/icons/application.png", [&] {
        const auto filePath =
                DialogUtil::getOpenFileName(tr("Icon for program"), tr("Icons (*.ico; *.png)"));

        if (filePath.isEmpty())
            return;

        setIconPath(filePath);
    });

    m_btDeleteIcon =
            ControlUtil::createIconToolButton(":/icons/delete.png", [&] { setIconPath({}); });

    auto layout = ControlUtil::createHLayout();
    layout->addWidget(m_editNotes);
    layout->addWidget(m_btSetIcon, 0, Qt::AlignTop);
    layout->addWidget(m_btDeleteIcon, 0, Qt::AlignTop);

    return layout;
}

QLayout *ProgramEditDialogBase::setupApplyChildGroupLayout()
{
    // Apply Child
    m_comboApplyChild =
            ControlUtil::createComboBox({}, [&](int /*index*/) { warnRestartNeededOption(); });
    m_comboApplyChild->setMinimumWidth(120);
    m_comboApplyChild->setMaximumWidth(200);

    setupCbApplyChild();

    // Group
    setupComboAppGroups();

    m_labelAppGroup = ControlUtil::createLabel();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbApplyChild,
            /*stretch*/ nullptr, m_comboApplyChild, ControlUtil::createVSeparator(),
            m_labelAppGroup, /*stretch*/ nullptr, m_comboAppGroup });

    return layout;
}

void ProgramEditDialogBase::setupCbApplyChild()
{
    m_cbApplyChild = ControlUtil::createCheckBox(":/icons/document_tree.png");

    const auto refreshApplyChildEnabled = [&](bool checked) {
        m_comboApplyChild->setEnabled(checked);
    };

    refreshApplyChildEnabled(false);

    connect(m_cbApplyChild, &QCheckBox::toggled, this, refreshApplyChildEnabled);
}

void ProgramEditDialogBase::setupComboAppGroups()
{
    m_comboAppGroup = ControlUtil::createComboBox();
    m_comboAppGroup->setMinimumWidth(120);
    m_comboAppGroup->setMaximumWidth(150);

    const auto refreshComboAppGroups = [&](bool onlyFlags = false) {
        if (onlyFlags)
            return;

        ControlUtil::setComboBoxTexts(m_comboAppGroup, conf()->appGroupNames(), /*currentIndex=*/0);
    };

    refreshComboAppGroups();

    connect(confManager(), &ConfManager::confChanged, this, refreshComboAppGroups);
}

QLayout *ProgramEditDialogBase::setupActionsLayout()
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

    connect(m_rbKillProcess, &QRadioButton::clicked, this,
            &ProgramEditDialogBase::warnDangerousOption);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { /*stretch*/ nullptr, m_rbAllow, m_rbBlock, m_rbKillProcess, /*stretch*/ nullptr });
    layout->setSpacing(20);

    return layout;
}

void ProgramEditDialogBase::setupActionsGroup()
{
    m_btgActions = new QButtonGroup(this);
    m_btgActions->setExclusive(true);

    m_btgActions->addButton(m_rbAllow, int(App::ScheduleAllow));
    m_btgActions->addButton(m_rbBlock, int(App::ScheduleBlock));
    m_btgActions->addButton(m_rbKillProcess, int(App::ScheduleKillProcess));

    connect(m_rbAllow, &QRadioButton::toggled, this,
            &ProgramEditDialogBase::updateZonesRulesLayout);
}

QLayout *ProgramEditDialogBase::setupZonesRuleLayout()
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

QLayout *ProgramEditDialogBase::setupRuleLayout()
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

QLayout *ProgramEditDialogBase::setupScheduleLayout()
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

    // Quick Actions
    setupQuickAction();
    setupTimedMenu();
    setupTimedAction();
    setupTimedRemove();

    // Schedule Check Box
    setupCbSchedule();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbSchedule, m_comboScheduleAction,
            m_comboScheduleType, /*stretch*/ nullptr, m_scScheduleIn, m_dteScheduleAt,
            m_btQuickAction, m_btTimedAction, m_btTimedRemove });

    return layout;
}

void ProgramEditDialogBase::setupCbSchedule()
{
    m_cbSchedule = ControlUtil::createCheckBox(":/icons/time.png");

    const auto refreshScheduleEnabled = [&](bool checked) {
        m_comboScheduleAction->setVisible(checked);
        m_comboScheduleType->setVisible(checked);

        m_scScheduleIn->setVisible(checked && m_scScheduleIn->isEnabled());
        m_dteScheduleAt->setVisible(checked && m_dteScheduleAt->isEnabled());

        m_btQuickAction->setVisible(!checked);
        m_btTimedAction->setVisible(!checked);
        m_btTimedRemove->setVisible(!checked);
    };

    refreshScheduleEnabled(false);

    connect(m_cbSchedule, &QCheckBox::toggled, this, refreshScheduleEnabled);
}

void ProgramEditDialogBase::setupComboScheduleType()
{
    m_comboScheduleType = ControlUtil::createComboBox();

    connect(m_comboScheduleType, &QComboBox::currentIndexChanged, this, [&](int index) {
        const bool isTimeIn = (index == ScheduleTimeIn);
        const bool isTimeAt = (index == ScheduleTimeAt);

        m_scScheduleIn->setEnabled(isTimeIn);
        m_dteScheduleAt->setEnabled(isTimeAt);

        const bool checked = m_cbSchedule->isChecked();

        m_scScheduleIn->setVisible(checked && isTimeIn);
        m_dteScheduleAt->setVisible(checked && isTimeAt);
    });
}

void ProgramEditDialogBase::setupQuickAction()
{
    m_btQuickAction = ControlUtil::createButton({}, [&] {
        selectQuickAction();

        m_btOk->click();
    });

    const auto refreshQuickActionType = [&](bool checked) {
        m_quickActionType = checked ? App::ScheduleBlock : App::ScheduleAllow;

        updateQuickAction();
    };

    connect(m_rbAllow, &QRadioButton::toggled, this, refreshQuickActionType);
}

void ProgramEditDialogBase::setupTimedMenu()
{
    m_timedMenu = ControlUtil::createMenu(this);

    m_timedMenuActions = new QActionGroup(m_timedMenu);

    connect(m_timedMenu, &QMenu::triggered, this, [&](QAction *action) {
        if (!m_timedMenuOwner)
            return;

        const int minutes = action->data().toInt();

        setTimedActionMinutes(m_timedMenuOwner, minutes);
        retranslateTimedAction(m_timedMenuOwner);
    });

    connect(
            m_timedMenu, &QMenu::aboutToHide, this, [&] { m_timedMenuOwner = nullptr; },
            Qt::QueuedConnection);
}

void ProgramEditDialogBase::setupTimedMenuActions()
{
    if (!m_timedMenuActions->actions().isEmpty())
        return;

    for (int minutes : appBlockInMinuteValues) {
        auto a = m_timedMenu->addAction(QString());
        a->setCheckable(true);
        a->setData(minutes);

        m_timedMenuActions->addAction(a);
    }

    retranslateTimedMenuActions();
}

void ProgramEditDialogBase::setupTimedAction()
{
    m_btTimedAction = createTimedButton(iniUser()->progAlertWindowTimedActionMinutesKey());

    connect(m_btTimedAction, &QToolButton::clicked, this, [&] {
        const auto currentActionType = App::ScheduleAction(m_btgActions->checkedId());

        selectQuickAction();

        const int minutes = timedActionMinutes(m_btTimedAction);

        saveScheduleAction(currentActionType, minutes);
    });
}

void ProgramEditDialogBase::setupTimedRemove()
{
    m_btTimedRemove = createTimedButton(iniUser()->progAlertWindowTimedRemoveMinutesKey());
    m_btTimedRemove->setIcon(IconCache::icon(":/icons/delete.png"));

    connect(m_btTimedRemove, &QToolButton::clicked, this, [&] {
        const int minutes = timedActionMinutes(m_btTimedRemove);

        saveScheduleAction(App::ScheduleRemove, minutes);
    });
}

QToolButton *ProgramEditDialogBase::createTimedButton(const QString &iniKey)
{
    auto bt = new ToolButton();
    bt->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    bt->setPopupMode(QToolButton::MenuButtonPopup);
    bt->setMenu(m_timedMenu);

    VariantUtil::setUserData(bt, iniKey);

    connect(bt, &ToolButton::aboutToShowMenu, this, [&] {
        auto bt = qobject_cast<QToolButton *>(sender());

        const int minutes = timedActionMinutes(bt);
        const int index = m_scScheduleIn->getIndexByValue(minutes);

        selectTimedMenuAction(index);

        m_timedMenuOwner = bt;
    });

    return bt;
}

QLayout *ProgramEditDialogBase::setupButtonsLayout()
{
    // Switch Wildcard
    setupSwitchWildcard();

    // Options
    setupOptions();

    // Connections
    setupConnections();

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

    auto layout =
            ControlUtil::createHLayoutByWidgets({ m_btSwitchWildcard, m_btOptions, m_btConnections,
                    /*stretch*/ nullptr, m_btOk, m_btCancel, m_btMenu });

    return layout;
}

void ProgramEditDialogBase::setupSwitchWildcard()
{
    m_btSwitchWildcard = ControlUtil::createIconToolButton(":/icons/coding.png", [&] {
        m_isWildcard = !m_isWildcard;

        switchWildcardPaths();

        updateWildcard();
        retranslateUi();
    });

    m_btSwitchWildcard->setCheckable(true);
}

void ProgramEditDialogBase::setupOptions()
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

void ProgramEditDialogBase::setupChildOptionsLayout()
{
    // Kill Child
    m_cbKillChild = ControlUtil::createCheckBox(":/icons/scull.png");

    connect(m_cbKillChild, &QCheckBox::clicked, this, &ProgramEditDialogBase::warnDangerousOption);
}

void ProgramEditDialogBase::setupLogOptions()
{
    // Log Allowed Connections
    m_cbLogAllowedConn = new QCheckBox();

    // Log Blocked Connections
    m_cbLogBlockedConn = new QCheckBox();
}

void ProgramEditDialogBase::setupConnections()
{
    m_connectionsLayout = new QVBoxLayout();

    auto menu = ControlUtil::createMenuByLayout(m_connectionsLayout, this);

    m_btConnections = ControlUtil::createButton(":/icons/connect.png");
    m_btConnections->setShortcut(QKeyCombination(Qt::CTRL | Qt::ALT, Qt::Key_C));
    m_btConnections->setMenu(menu);

    connect(menu, &QMenu::aboutToShow, this, &ProgramEditDialogBase::setupConnectionsMenuLayout);
    connect(menu, &QMenu::aboutToHide, this, &ProgramEditDialogBase::closeConnectionsMenuLayout);
}

void ProgramEditDialogBase::setupConnectionsMenuLayout()
{
    Q_ASSERT(m_connectionsLayout->isEmpty());

    setupConnectionsModel();
    setupTableConnList();
    setupTableConnListHeader();

    setupTableConnsChanged();

    retranslateTableConnListMenu();

    m_connListView->setFixedWidth(800);

    m_connectionsLayout->addWidget(m_connListView);
}

void ProgramEditDialogBase::closeConnectionsMenuLayout()
{
    ControlUtil::clearLayout(m_connectionsLayout);

    m_appConnListModel->deleteLater();
}

void ProgramEditDialogBase::setupConnectionsModel()
{
    m_appConnListModel = new AppConnListModel(this);

    appConnListModel()->setAppPath(m_app.appPath);
    appConnListModel()->setResolveAddress(true);
    appConnListModel()->initialize();
}

void ProgramEditDialogBase::setupTableConnList()
{
    m_connListView = new TableView();
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_connListView->setModel(appConnListModel());

    setupTableConnListMenu();
}

void ProgramEditDialogBase::setupTableConnListMenu()
{
    auto menu = ControlUtil::createMenu(m_connListView);

    m_actCopyAsFilter = menu->addAction(IconCache::icon(":/icons/script.png"), QString());
    m_actCopyAsFilter->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C);

    m_actCopy = menu->addAction(IconCache::icon(":/icons/page_copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actLookupIp = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actLookupIp->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_L);

    connect(m_actCopyAsFilter, &QAction::triggered, this, [&] {
        const auto rows = m_connListView->selectedRows();
        const auto text = appConnListModel()->rowsAsFilter(rows);

        GuiUtil::setClipboardData(text);
    });
    connect(m_actCopy, &QAction::triggered, m_connListView, &TableView::copySelectedText);
    connect(m_actLookupIp, &QAction::triggered, this, [&] {
        const auto row = m_connListView->currentRow();
        const auto index = appConnListModel()->index(row, int(ConnListColumn::RemoteIp));
        const auto textIp = appConnListModel()->data(index).toString();

        OsUtil::openIpLocationUrl(textIp);
    });

    m_connListView->setMenu(menu);

    m_connListView->addAction(m_actCopyAsFilter);
    m_connListView->addAction(m_actLookupIp);
}

void ProgramEditDialogBase::setupTableConnListHeader()
{
    auto header = m_connListView->horizontalHeader();

    header->setSectionResizeMode(int(ConnListColumn::Program), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::ProcessId), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::Protocol), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalHostName), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalIp), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalPort), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemoteHostName), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemoteIp), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemotePort), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::Direction), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Action), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Reason), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Time), QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(int(ConnListColumn::Program), 300);
    header->resizeSection(int(ConnListColumn::ProcessId), 60);
    header->resizeSection(int(ConnListColumn::Protocol), 60);
    header->resizeSection(int(ConnListColumn::LocalHostName), 140);
    header->resizeSection(int(ConnListColumn::LocalIp), 100);
    header->resizeSection(int(ConnListColumn::LocalPort), 60);
    header->resizeSection(int(ConnListColumn::RemoteHostName), 150);
    header->resizeSection(int(ConnListColumn::RemoteIp), 100);
    header->resizeSection(int(ConnListColumn::RemotePort), 70);
    header->resizeSection(int(ConnListColumn::Direction), 30);
    header->resizeSection(int(ConnListColumn::Action), 30);
    header->resizeSection(int(ConnListColumn::Reason), 30);
    header->resizeSection(int(ConnListColumn::Time), 80);

    // Hidden columns
    header->setSectionHidden(int(ConnListColumn::Program), /*hide=*/true);
    header->setSectionHidden(int(ConnListColumn::LocalHostName), /*hide=*/true);
}

void ProgramEditDialogBase::setupTableConnsChanged()
{
    const auto refreshTableConnsChanged = [&] {
        const int connIndex = m_connListView->currentRow();
        const bool connSelected = (connIndex >= 0);
        m_actCopyAsFilter->setEnabled(connSelected);
        m_actCopy->setEnabled(connSelected);
        m_actLookupIp->setEnabled(connSelected);
    };

    refreshTableConnsChanged();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshTableConnsChanged);
}

void ProgramEditDialogBase::updateZonesRulesLayout()
{
    const bool enabled = m_rbAllow->isChecked();

    m_cbLanOnly->setEnabled(enabled);
    m_btZones->setEnabled(enabled);

    const bool isSingleSelection = (m_appIdList.size() <= 1);
    if (!isSingleSelection)
        return;

    m_btSelectRule->setEnabled(enabled);
}

void ProgramEditDialogBase::updateApplyChild()
{
    const ApplyChildType type = m_app.applyParent ? ApplyChildType::FromParent
            : m_app.applySpecChild                ? ApplyChildType::ToSpecChild
            : m_app.applyChild                    ? ApplyChildType::ToChild
                                                  : ApplyChildType::Invalid;
    const bool hasApplyChild = (type != ApplyChildType::Invalid);

    m_cbApplyChild->setChecked(hasApplyChild);
    m_comboApplyChild->setCurrentIndex(int(hasApplyChild ? type : ApplyChildType::ToChild));
}

void ProgramEditDialogBase::updateWildcard()
{
    m_editPath->setVisible(!isWildcard());

    m_editWildcard->setVisible(isWildcard());

    updateAppIcon();
}

void ProgramEditDialogBase::updateAppIcon()
{
    QPixmap pixmap;

    if (!m_iconPath.isEmpty()) {
        pixmap = IconCache::pixmap(m_iconPath, appIconSize);
    } else {
        const bool isSingleSelection = m_labelEditNotes->isEnabled();

        pixmap = appIcon(isSingleSelection).pixmap(appIconSize);
    }

    m_labelEditNotes->setPixmap(pixmap);
}

void ProgramEditDialogBase::updateQuickAction()
{
    auto rb = m_btgActions->button(int(m_quickActionType));

    m_btQuickAction->setIcon(rb->icon());
    m_btQuickAction->setText(rb->text());

    m_btTimedAction->setIcon(rb->icon());
}

void ProgramEditDialogBase::selectQuickAction()
{
    auto rb = m_btgActions->button(int(m_quickActionType));
    rb->setChecked(true);
}

void ProgramEditDialogBase::selectTimedMenuAction(int index)
{
    setupTimedMenuActions();

    auto a = m_timedMenuActions->actions().at(index);
    a->setChecked(true);
}

int ProgramEditDialogBase::timedActionMinutes(QToolButton *bt)
{
    constexpr int defaultMinutes = 5;

    const auto iniKey = VariantUtil::userData(bt).toString();
    return iniUser()->valueInt(iniKey, defaultMinutes);
}

void ProgramEditDialogBase::setTimedActionMinutes(QToolButton *bt, int minutes)
{
    const auto iniKey = VariantUtil::userData(bt).toString();
    iniUser()->setValue(iniKey, minutes);
}

void ProgramEditDialogBase::switchWildcardPaths()
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

void ProgramEditDialogBase::fillEditName()
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

QString ProgramEditDialogBase::getEditText() const
{
    return isWildcard() ? m_editWildcard->toPlainText() : m_editPath->text();
}

void ProgramEditDialogBase::setIconPath(const QString &iconPath)
{
    m_iconPath = iconPath;

    const bool hasIconPath = !m_iconPath.isEmpty();

    m_btSetIcon->setVisible(!hasIconPath);
    m_btDeleteIcon->setVisible(hasIconPath);

    updateAppIcon();
}

void ProgramEditDialogBase::selectRuleDialog()
{
    auto rulesDialog = RulesWindow::showRulesDialog(Rule::AppRule, this);

    connect(rulesDialog, &RulesWindow::ruleSelected, this, [&](const RuleRow &ruleRow) {
        setCurrentRuleId(ruleRow.ruleId);
        m_editRuleName->setStartText(ruleRow.ruleName);
    });
}

void ProgramEditDialogBase::editRuleDialog(int ruleId)
{
    RulesWindow::showRuleEditDialog(ruleId, Rule::AppRule, this);
}

void ProgramEditDialogBase::warnDangerousOption() const
{
    IoC<WindowManager>()->showErrorBox(
            tr("Attention: This option is very dangerous!!!\n\n"
               "Be careful when killing a system services or other important programs!\n"
               "It can cause a Windows malfunction or totally unusable."));
}

void ProgramEditDialogBase::warnRestartNeededOption() const
{
    IoC<WindowManager>()->showErrorBox(
            tr("Attention: This option only affects new processes!\n\n"
               "Please restart the running program to take effect of this option."));
}
