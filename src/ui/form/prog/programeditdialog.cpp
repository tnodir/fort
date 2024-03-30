#include "programeditdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/spincombo.h>
#include <form/controls/zonesselector.h>
#include <form/dialog/dialogutil.h>
#include <fortmanager.h>
#include <manager/windowmanager.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/textareautil.h>

#include "programscontroller.h"

namespace {

const std::array appBlockInMinuteValues = { 15, 0, 1, 5, 10, 30, 60 * 1, 60 * 6, 60 * 12, 60 * 24,
    60 * 24 * 7, 60 * 24 * 30 };

}

enum ScheduleTimeType : qint8 {
    ScheduleTimeIn = 0,
    ScheduleTimeAt,
};

ProgramEditDialog::ProgramEditDialog(ProgramsController *ctrl, QWidget *parent, Qt::WindowFlags f) :
    WidgetWindow(parent, (f == Qt::Widget ? Qt::Dialog : f)), m_ctrl(ctrl)
{
    setupUi();
    setupController();
}

FortManager *ProgramEditDialog::fortManager() const
{
    return ctrl()->fortManager();
}

ConfAppManager *ProgramEditDialog::confAppManager() const
{
    return ctrl()->confAppManager();
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
    m_appRow = appRow;
    m_appIdList = appIdList;

    retranslateUi();

    initializePathNameFields();

    m_comboAppGroup->setCurrentIndex(appRow.groupIndex);

    m_rbAllow->setChecked(!appRow.blocked);
    m_rbBlock->setChecked(appRow.blocked);
    m_rbKillProcess->setChecked(appRow.killProcess);

    m_cbUseGroupPerm->setChecked(appRow.useGroupPerm);
    m_cbApplyChild->setChecked(appRow.applyChild);
    m_cbKillChild->setChecked(appRow.killChild);

    m_cbParked->setChecked(appRow.parked);
    m_cbParked->setEnabled(!isWildcard());
    m_cbLogBlocked->setChecked(appRow.logBlocked);
    m_cbLogConn->setChecked(appRow.logConn);

    m_cbLanOnly->setChecked(appRow.lanOnly);
    m_btZones->setZones(appRow.acceptZones);
    m_btZones->setUncheckedZones(appRow.rejectZones);

    m_cbSchedule->setChecked(!appRow.scheduleTime.isNull());
    m_comboScheduleAction->setCurrentIndex(appRow.scheduleAction);
    m_comboScheduleType->setCurrentIndex(
            appRow.scheduleTime.isNull() ? ScheduleTimeIn : ScheduleTimeAt);
    m_scScheduleIn->spinBox()->setValue(30);
    m_dteScheduleAt->setDateTime(appRow.scheduleTime);
    m_dteScheduleAt->setMinimumDateTime(DateUtil::now());

    initializeFocus();
}

void ProgramEditDialog::initializePathNameFields()
{
    const bool isSingleSelection = (m_appIdList.size() <= 1);
    const bool isPathEditable = isSingleSelection && (m_appRow.appId == 0 || isWildcard());

    initializePathField(isSingleSelection, isPathEditable);
    initializeNameField(isSingleSelection);
}

void ProgramEditDialog::initializePathField(bool isSingleSelection, bool isPathEditable)
{
    m_editPath->setText(isSingleSelection && !isWildcard() ? m_appRow.appOriginPath : QString());
    m_editPath->setReadOnly(!isPathEditable);
    m_editPath->setClearButtonEnabled(isPathEditable);
    m_editPath->setEnabled(isSingleSelection);
    m_editPath->setVisible(!isWildcard());

    m_editWildcard->setText(isSingleSelection && isWildcard() ? m_appRow.appOriginPath : QString());
    m_editWildcard->setReadOnly(!isPathEditable);
    m_editWildcard->setEnabled(isSingleSelection);
    m_editWildcard->setVisible(isWildcard());

    m_btSelectFile->setEnabled(isSingleSelection);
}

void ProgramEditDialog::initializeNameField(bool isSingleSelection)
{
    m_editName->setText(isSingleSelection ? m_appRow.appName : QString());
    m_editName->setEnabled(isSingleSelection);
    m_editName->setClearButtonEnabled(isSingleSelection);

    m_btGetName->setEnabled(isSingleSelection);

    m_editNotes->setText(m_appRow.notes);
    m_editNotes->setEnabled(isSingleSelection);

    m_labelEditNotes->setPixmap(appIcon(isSingleSelection));

    if (isSingleSelection) {
        if (m_appRow.appName.isEmpty()) {
            fillEditName(); // Auto-fill the name
        }
    }
}

void ProgramEditDialog::initializeFocus()
{
    if (!isEmpty()) {
        m_btgActions->checkedButton()->setFocus();
    } else if (isWildcard()) {
        m_editWildcard->setFocus();
    } else {
        m_editPath->selectAll();
        m_editPath->setFocus();
    }
}

QPixmap ProgramEditDialog::appIcon(bool isSingleSelection) const
{
    if (!isSingleSelection)
        return {};

    if (isWildcard())
        return IconCache::file(":/icons/coding.png");

    return IoC<AppInfoCache>()->appPixmap(m_appRow.appPath);
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

void ProgramEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelEditPath->setText(isWildcard() ? tr("Wildcard Paths:") : tr("File Path:"));
    retranslatePathPlaceholderText();
    m_btSelectFile->setToolTip(tr("Select File"));

    m_labelEditName->setText(tr("Name:"));
    m_btGetName->setToolTip(tr("Get Program Name"));

    m_editNotes->setPlaceholderText(tr("Notes"));
    m_labelAppGroup->setText(tr("Group:"));

    m_rbAllow->setText(tr("Allow"));
    m_rbBlock->setText(tr("Block"));
    m_rbKillProcess->setText(tr("Kill Process"));

    m_cbUseGroupPerm->setText(tr("Use Application Group's Enabled State"));
    m_cbApplyChild->setText(tr("Apply same rules to child processes"));
    m_cbKillChild->setText(tr("Kill child processes"));

    m_cbParked->setText(tr("Parked"));
    m_cbParked->setToolTip(tr("Don't purge as obsolete"));
    m_cbLogBlocked->setText(tr("Collect blocked connections"));
    m_cbLogConn->setText(tr("Collect connection statistics"));

    m_cbLanOnly->setText(tr("Block Internet Traffic"));
    m_btZones->retranslateUi();

    m_cbSchedule->setText(tr("Schedule"));
    retranslateScheduleAction();
    retranslateScheduleType();
    retranslateScheduleIn();
    m_dteScheduleAt->unsetLocale();

    m_btOptions->setToolTip(tr("Advanced Options"));
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

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Size
    this->setMinimumWidth(500);
}

QLayout *ProgramEditDialog::setupMainLayout()
{
    // Form Layout
    auto formLayout = setupFormLayout();

    // Allow/Block/Kill Actions Layout
    auto actionsLayout = setupActionsLayout();

    setupActionsGroup();

    // Schedule
    auto scheduleLayout = setupScheduleLayout();

    // Advanced Options
    setupAdvancedOptions();

    // Menu, OK/Cancel
    auto buttonsLayout = setupButtonsLayout();

    // Form
    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addStretch();
    layout->addLayout(actionsLayout);
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
    m_labelEditNotes->setFixedSize(32, 32);

    // Group
    setupComboAppGroups();

    layout->addRow("Group:", m_comboAppGroup);
    m_labelAppGroup = ControlUtil::formRowLabel(layout, m_comboAppGroup);

    return layout;
}

QLayout *ProgramEditDialog::setupPathLayout()
{
    auto layout = new QHBoxLayout();

    m_editPath = new QLineEdit();
    m_editPath->setMaxLength(1024);

    m_editWildcard = new PlainTextEdit();

    m_btSelectFile = ControlUtil::createIconToolButton(":/icons/folder.png", [&] {
        if (!isEmpty()) {
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

    layout->addWidget(m_editPath);
    layout->addWidget(m_editWildcard);
    layout->addWidget(m_btSelectFile, 0, Qt::AlignTop);

    return layout;
}

QLayout *ProgramEditDialog::setupNameLayout()
{
    m_editName = new QLineEdit();
    m_editName->setMaxLength(1024);

    m_btGetName = ControlUtil::createIconToolButton(
            ":/icons/arrow_refresh_small.png", [&] { fillEditName(); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_editName, m_btGetName });

    return layout;
}

void ProgramEditDialog::setupComboAppGroups()
{
    m_comboAppGroup = ControlUtil::createComboBox();

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
}

void ProgramEditDialog::setupAdvancedOptions()
{
    // Use Group Perm.
    m_cbUseGroupPerm = new QCheckBox();

    // Child Options
    auto childLayout = setupChildLayout();

    // Log
    auto logLayout = setupLogLayout();

    // Zones
    auto zonesLayout = setupZonesLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbUseGroupPerm);
    layout->addLayout(childLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(logLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(zonesLayout);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btOptions = ControlUtil::createButton(":/icons/widgets.png");
    m_btOptions->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_O));
    m_btOptions->setMenu(menu);
}

QLayout *ProgramEditDialog::setupChildLayout()
{
    // Apply Child
    m_cbApplyChild = new QCheckBox();

    // Kill Child
    m_cbKillChild = new QCheckBox();
    connect(m_cbKillChild, &QCheckBox::clicked, this, &ProgramEditDialog::warnDangerousOption);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbApplyChild, ControlUtil::createVSeparator(), m_cbKillChild });

    return layout;
}

QLayout *ProgramEditDialog::setupLogLayout()
{
    // Parked
    m_cbParked = new QCheckBox();

    // Log Blocked
    m_cbLogBlocked = new QCheckBox();

    // Log Conn
    m_cbLogConn = new QCheckBox();
    m_cbLogConn->setVisible(false); // TODO: Collect allowed connections

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbParked, ControlUtil::createVSeparator(),
            m_cbLogBlocked, m_cbLogConn, /*stretch*/ nullptr });

    return layout;
}

QLayout *ProgramEditDialog::setupZonesLayout()
{
    // LAN Only
    m_cbLanOnly = new QCheckBox();

    // Zones
    m_btZones = new ZonesSelector();
    m_btZones->setIsTristate(true);
    m_btZones->setMaxZoneCount(16); // sync with driver's FORT_APP_ENTRY

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbLanOnly, ControlUtil::createVSeparator(), m_btZones, /*stretch*/ nullptr });

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
    m_cbSchedule = new QCheckBox();

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
        const bool isTimeIn = (index == ScheduleTimeIn);
        m_scScheduleIn->setVisible(isTimeIn);
        m_dteScheduleAt->setVisible(!isTimeIn);
    });
}

QLayout *ProgramEditDialog::setupButtonsLayout()
{
    // Menu button
    m_btMenu = windowManager()->createMenuButton();

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

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btOptions);
    layout->addWidget(m_btMenu);
    layout->addWidget(m_btOk, 1, Qt::AlignRight);
    layout->addWidget(m_btCancel);

    return layout;
}

void ProgramEditDialog::fillEditName()
{
    auto appPath = isWildcard() ? m_editWildcard->toPlainText() : m_editPath->text();
    if (appPath.isEmpty())
        return;

    QString appName;
    if (isWildcard()) {
        appName = appPath.left(64).replace('\n', ' ');
    } else {
        appPath = FileUtil::normalizePath(appPath);
        appName = IoC<AppInfoCache>()->appName(appPath);
    }

    m_editName->setText(appName);
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
    for (qint64 appId : m_appIdList) {
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
    app.useGroupPerm = m_cbUseGroupPerm->isChecked();
    app.applyChild = m_cbApplyChild->isChecked();
    app.killChild = m_cbKillChild->isChecked();
    app.lanOnly = m_cbLanOnly->isChecked();
    app.parked = m_cbParked->isChecked();
    app.logBlocked = m_cbLogBlocked->isChecked();
    app.logConn = m_cbLogConn->isChecked();
    app.blocked = !m_rbAllow->isChecked();
    app.killProcess = m_rbKillProcess->isChecked();
    app.groupIndex = m_comboAppGroup->currentIndex();
    app.appName = m_editName->text();
    app.notes = m_editNotes->toPlainText();

    app.acceptZones = m_btZones->zones();
    app.rejectZones = m_btZones->uncheckedZones();

    fillAppPath(app);
    fillAppEndTime(app);
}

void ProgramEditDialog::fillAppPath(App &app) const
{
    const QString appPath = m_editPath->text();

    app.appOriginPath = isWildcard() ? m_editWildcard->toPlainText() : appPath;
    app.appPath = FileUtil::normalizePath(appPath);
}

void ProgramEditDialog::fillAppEndTime(App &app) const
{
    if (!m_cbSchedule->isChecked()) {
        app.scheduleAction = App::ScheduleBlock;
        app.scheduleTime = {};
        return;
    }

    app.scheduleAction = m_comboScheduleAction->currentIndex();

    if (m_comboScheduleType->currentIndex() == ScheduleTimeIn) {
        const int minutes = m_scScheduleIn->spinBox()->value();

        app.scheduleTime = DateUtil::now().addSecs(minutes * 60);
    } else {
        app.scheduleTime = m_dteScheduleAt->dateTime();
    }
}

bool ProgramEditDialog::isWildcard() const
{
    return m_appRow.isWildcard;
}

void ProgramEditDialog::warnDangerousOption() const
{
    IoC<WindowManager>()->showErrorBox(
            tr("Attention: This option is very dangerous!!!\n\n"
               "Be careful when killing a system services or other important programs!\n"
               "It can cause a Windows malfunction or totally unusable."));
}
