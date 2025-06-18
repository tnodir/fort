#include "programeditdialog.h"

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

ProgramEditDialog::ProgramEditDialog(
        ProgramEditController *ctrl, QWidget *parent, Qt::WindowFlags f) :
    ProgramEditDialogBase(ctrl, parent, f)
{
    setupUi();
    setupController();
    setupRuleManager();
}

void ProgramEditDialog::initialize(const App &app, const QVector<qint64> &appIdList)
{
    const bool isSingleSelection = (appIdList.size() <= 1);

    m_isWildcard = app.isWildcard;
    m_app = app;
    m_appIdList = appIdList;

    retranslateUi();

    initializePathNameRuleFields(isSingleSelection);

    updateApplyChild();
    m_comboAppGroup->setCurrentIndex(app.groupIndex);

    m_rbAllow->setChecked(!app.blocked);
    m_rbBlock->setChecked(app.blocked);
    m_rbKillProcess->setChecked(app.killProcess);

    m_cbKillChild->setChecked(app.killChild);

    m_cbParked->setChecked(app.parked);
    m_cbLogAllowedConn->setChecked(app.logAllowedConn);
    m_cbLogBlockedConn->setChecked(app.logBlockedConn);

    m_cbLanOnly->setChecked(app.lanOnly);
    m_btZones->setZones(app.zones.accept_mask);
    m_btZones->setUncheckedZones(app.zones.reject_mask);
    updateZonesRulesLayout();

    const bool hasScheduleTime = !app.scheduleTime.isNull();
    const ScheduleType scheduleType = hasScheduleTime ? ScheduleTimeAt : ScheduleTimeIn;

    m_cbSchedule->setChecked(hasScheduleTime);
    m_comboScheduleAction->setCurrentIndex(app.scheduleAction);
    m_comboScheduleType->setCurrentIndex(scheduleType);
    m_scScheduleIn->spinBox()->setValue(5);
    m_dteScheduleAt->setDateTime(app.scheduleTime);
    m_dteScheduleAt->setMinimumDateTime(DateUtil::now());

    m_btSwitchWildcard->setChecked(isWildcard());
    m_btSwitchWildcard->setEnabled(isSingleSelection);

    updateWildcard();

    initializeFocus();
}

void ProgramEditDialog::initializePathNameRuleFields(bool isSingleSelection)
{
    initializePathField(isSingleSelection);
    initializeNameField(isSingleSelection);
    initializeNotesField(isSingleSelection);
    initializeRuleField(isSingleSelection);
}

void ProgramEditDialog::initializePathField(bool isSingleSelection)
{
    const bool isPathEditable = isSingleSelection && (isNew() || isWildcard());

    m_editPath->setReadOnly(!isPathEditable);
    m_editPath->setClearButtonEnabled(isPathEditable);

    m_editPath->setText(isSingleSelection && !isWildcard() ? m_app.appOriginPath : QString());
    m_editPath->setEnabled(isSingleSelection);

    m_editWildcard->setText(isSingleSelection && isWildcard() ? m_app.appOriginPath : QString());
    m_editWildcard->setEnabled(isSingleSelection);

    m_btSelectFile->setEnabled(isSingleSelection);
}

void ProgramEditDialog::initializeNameField(bool isSingleSelection)
{
    m_editName->setStartText(isSingleSelection ? m_app.appName : QString());
    m_editName->setEnabled(isSingleSelection);
    m_editName->setClearButtonEnabled(isSingleSelection);

    m_btGetName->setEnabled(isSingleSelection);

    if (isSingleSelection) {
        if (m_app.appName.isEmpty()) {
            fillEditName(); // Auto-fill the name
        }
    }
}

void ProgramEditDialog::initializeNotesField(bool isSingleSelection)
{
    m_labelEditNotes->setEnabled(isSingleSelection);

    m_editNotes->setText(m_app.notes);
    m_editNotes->setEnabled(isSingleSelection);

    m_btSetIcon->setEnabled(isSingleSelection);
    m_btDeleteIcon->setEnabled(isSingleSelection);

    setIconPath(m_app.iconPath);
}

void ProgramEditDialog::initializeRuleField(bool isSingleSelection)
{
    setCurrentRuleId(m_app.ruleId);

    m_editRuleName->setStartText(
            isSingleSelection ? confRuleManager()->ruleNameById(m_app.ruleId) : QString());
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

void ProgramEditDialog::setupUi()
{
    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/application.png"));

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Size
    this->setMinimumWidth(500);
}

void ProgramEditDialog::saveScheduleAction(App::ScheduleAction actionType, int minutes)
{
    m_cbSchedule->setChecked(true);
    m_comboScheduleAction->setCurrentIndex(int(actionType));
    m_comboScheduleType->setCurrentIndex(ScheduleTimeIn);
    m_scScheduleIn->spinBox()->setValue(minutes);

    m_btOk->click();
}

bool ProgramEditDialog::save()
{
    const int appIdsCount = m_appIdList.size();
    const bool isSingleSelection = (appIdsCount <= 1);

    if (isSingleSelection && !validateFields())
        return false;

    App app;
    app.appId = m_app.appId;
    fillApp(app);

    // Add new app or edit non-selected app
    if (appIdsCount == 0) {
        const bool onlyUpdate = app.isValid();

        return ctrl()->addOrUpdateApp(app, onlyUpdate);
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
    if (!app.isOptionsEqual(m_app)) {
        return ctrl()->updateApp(app);
    }

    if (!app.isNameEqual(m_app)) {
        return ctrl()->updateAppName(app.appId, app.appName);
    }

    return true;
}

bool ProgramEditDialog::saveMulti(App &app)
{
    for (qint64 appId : std::as_const(m_appIdList)) {
        const auto appRow = confAppManager()->appById(appId);

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
    app.iconPath = m_iconPath;

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
