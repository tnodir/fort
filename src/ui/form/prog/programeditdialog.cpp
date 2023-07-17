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
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/checkspincombo.h>
#include <form/controls/controlutil.h>
#include <form/dialog/dialogutil.h>
#include <fortmanager.h>
#include <manager/windowmanager.h>
#include <util/fileutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/window/widgetwindow.h>

#include "programscontroller.h"

namespace {

const std::array appBlockInHourValues = { 3, 1, 6, 12, 24, 24 * 7, 24 * 30 };

}

ProgramEditDialog::ProgramEditDialog(ProgramsController *ctrl, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl)
{
    setupUi();
    setupController();
}

FortManager *ProgramEditDialog::fortManager() const
{
    return ctrl()->fortManager();
}

ConfManager *ProgramEditDialog::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *ProgramEditDialog::conf() const
{
    return ctrl()->conf();
}

AppListModel *ProgramEditDialog::appListModel() const
{
    return ctrl()->appListModel();
}

void ProgramEditDialog::initialize(const AppRow &appRow, const QVector<qint64> &appIdList)
{
    m_appRow = appRow;
    m_appIdList = appIdList;

    const bool isSingleSelection = (appIdList.size() <= 1);
    const bool isPathEditable = isSingleSelection && appRow.appId == 0;

    m_editPath->setText(isSingleSelection ? appRow.appOriginPath : QString());
    m_editPath->setReadOnly(!isPathEditable);
    m_editPath->setClearButtonEnabled(isPathEditable);
    m_editPath->setEnabled(isSingleSelection);
    m_btSelectFile->setEnabled(isPathEditable);
    m_editName->setText(isSingleSelection ? appRow.appName : QString());
    m_editName->setEnabled(isSingleSelection);
    m_editName->setClearButtonEnabled(isSingleSelection);
    m_btGetName->setEnabled(isSingleSelection);
    m_comboAppGroup->setCurrentIndex(appRow.groupIndex);
    m_cbUseGroupPerm->setChecked(appRow.useGroupPerm);
    m_cbApplyChild->setChecked(appRow.applyChild);
    m_cbLanOnly->setChecked(appRow.lanOnly);
    m_cbLogBlocked->setChecked(appRow.logBlocked);
    m_cbLogConn->setChecked(appRow.logConn);
    m_rbAllowApp->setChecked(!appRow.blocked);
    m_rbBlockApp->setChecked(appRow.blocked);
    m_rbKillProcess->setChecked(appRow.killProcess);
    m_cscBlockAppIn->checkBox()->setChecked(false);
    m_cscBlockAppIn->spinBox()->setValue(1);
    m_cbBlockAppAt->setChecked(!appRow.endTime.isNull());
    m_dteBlockAppAt->setDateTime(appRow.endTime);
    m_dteBlockAppAt->setMinimumDateTime(QDateTime::currentDateTime());
    m_cbBlockAppNone->setChecked(appRow.endTime.isNull());

    if (isSingleSelection && appRow.appName.isEmpty()) {
        m_btGetName->click(); // Auto-fill the name
    }
}

void ProgramEditDialog::activate()
{
    WidgetWindow::showWidget(this);

    m_editPath->selectAll();
    m_editPath->setFocus();
}

void ProgramEditDialog::setupController()
{
    connect(ctrl(), &ProgramsController::retranslateUi, this, &ProgramEditDialog::retranslateUi);

    retranslateUi();
}

void ProgramEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelEditPath->setText(tr("File Path:"));
    m_btSelectFile->setToolTip(tr("Select File"));
    m_labelEditName->setText(tr("Name:"));
    m_btGetName->setToolTip(tr("Get Program Name"));

    m_labelAppGroup->setText(tr("Application Group:"));
    m_cbUseGroupPerm->setText(tr("Use Application Group's Enabled State"));
    m_cbApplyChild->setText(tr("Apply same rules to child processes"));
    m_cbLanOnly->setText(tr("Restrict access to LAN only"));

    m_cbLogBlocked->setText(tr("Collect blocked connections"));
    m_cbLogConn->setText(tr("Collect connection statistics"));

    m_rbAllowApp->setText(tr("Allow"));
    m_rbBlockApp->setText(tr("Block"));
    m_rbKillProcess->setText(tr("Kill Process"));

    m_cscBlockAppIn->checkBox()->setText(tr("Block In:"));
    retranslateAppBlockInHours();
    m_cbBlockAppAt->setText(tr("Block At:"));
    m_dteBlockAppAt->unsetLocale();
    m_cbBlockAppNone->setText(tr("Forever"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Program"));
}

void ProgramEditDialog::retranslateAppBlockInHours()
{
    const QStringList list = { tr("Custom"), tr("1 hour"), tr("6 hours"), tr("12 hours"), tr("Day"),
        tr("Week"), tr("Month") };

    m_cscBlockAppIn->setNames(list);
    m_cscBlockAppIn->spinBox()->setSuffix(tr(" hour(s)"));
}

void ProgramEditDialog::setupUi()
{
    // Form Layout
    auto formLayout = setupAppLayout();

    // Log
    auto logLayout = setupLogLayout();

    // Allow/Block
    auto allowLayout = setupAllowLayout();

    // Block at specified date & time
    auto blockAtLayout = setupCheckDateTimeEdit();

    // Eclusive End Time CheckBoxes Group
    setupAllowEclusiveGroup();

    // OK/Cancel
    auto buttonsLayout = new QHBoxLayout();

    m_btOk = ControlUtil::createButton(QString(), [&] {
        if (save()) {
            this->close();
        }
    });
    m_btOk->setDefault(true);

    m_btCancel = new QPushButton();
    connect(m_btCancel, &QAbstractButton::clicked, this, &QWidget::close);

    buttonsLayout->addWidget(m_btOk, 1, Qt::AlignRight);
    buttonsLayout->addWidget(m_btCancel);

    // Form
    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(logLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(allowLayout);
    layout->addWidget(m_cscBlockAppIn);
    layout->addLayout(blockAtLayout);
    layout->addWidget(m_cbBlockAppNone);
    layout->addStretch();
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality & Size Grip
    this->setWindowModality(Qt::WindowModal);
    this->setSizeGripEnabled(true);

    // Size
    this->setMinimumWidth(500);
}

QLayout *ProgramEditDialog::setupAppLayout()
{
    auto layout = new QFormLayout();

    // App Path
    auto pathLayout = setupAppPathLayout();

    layout->addRow("Program Path:", pathLayout);
    m_labelEditPath = qobject_cast<QLabel *>(layout->labelForField(pathLayout));

    // App Name
    auto nameLayout = setupAppNameLayout();

    layout->addRow("Program Name:", nameLayout);
    m_labelEditName = qobject_cast<QLabel *>(layout->labelForField(nameLayout));

    // App Group
    setupComboAppGroups();

    layout->addRow("Application Group:", m_comboAppGroup);
    m_labelAppGroup = qobject_cast<QLabel *>(layout->labelForField(m_comboAppGroup));

    // Use Group Perm.
    m_cbUseGroupPerm = new QCheckBox();

    layout->addRow(QString(), m_cbUseGroupPerm);

    // Apply Child
    m_cbApplyChild = new QCheckBox();

    layout->addRow(QString(), m_cbApplyChild);

    // LAN Only
    m_cbLanOnly = new QCheckBox();

    layout->addRow(QString(), m_cbLanOnly);

    return layout;
}

QLayout *ProgramEditDialog::setupAppPathLayout()
{
    auto layout = new QHBoxLayout();

    m_editPath = new QLineEdit();
    m_editPath->setMaxLength(1024);

    m_btSelectFile = ControlUtil::createFlatToolButton(":/icons/folder.png", [&] {
        const auto filePath = DialogUtil::getOpenFileName(
                m_labelEditPath->text(), tr("Programs (*.exe);;All files (*.*)"));

        if (!filePath.isEmpty()) {
            m_editPath->setText(filePath);
            m_btGetName->click(); // Auto-fill the name
        }
    });

    layout->addWidget(m_editPath);
    layout->addWidget(m_btSelectFile);

    return layout;
}

QLayout *ProgramEditDialog::setupAppNameLayout()
{
    auto layout = new QHBoxLayout();

    m_editName = new QLineEdit();

    const auto updateAppName = [&] {
        const auto appPath = m_editPath->text();
        if (appPath.isEmpty())
            return;

        const QString appName = IoC<AppInfoCache>()->appName(appPath);
        m_editName->setText(appName);
    };

    m_btGetName =
            ControlUtil::createFlatToolButton(":/icons/arrow_refresh_small.png", updateAppName);

    layout->addWidget(m_editName);
    layout->addWidget(m_btGetName);

    return layout;
}

void ProgramEditDialog::setupComboAppGroups()
{
    m_comboAppGroup = ControlUtil::createComboBox();

    const auto refreshComboAppGroups = [&](bool onlyFlags = false) {
        if (onlyFlags)
            return;

        m_comboAppGroup->clear();
        m_comboAppGroup->addItems(conf()->appGroupNames());
        m_comboAppGroup->setCurrentIndex(0);
    };

    refreshComboAppGroups();

    connect(confManager(), &ConfManager::confChanged, this, refreshComboAppGroups);
}

QLayout *ProgramEditDialog::setupLogLayout()
{
    auto layout = new QFormLayout();

    // Log Blocked
    m_cbLogBlocked = new QCheckBox();

    layout->addRow(QString(), m_cbLogBlocked);

    // Log Conn
    m_cbLogConn = new QCheckBox();

    m_cbLogConn->setVisible(false); // TODO: Collect allowed connections

    layout->addRow(QString(), m_cbLogConn);

    return layout;
}

QLayout *ProgramEditDialog::setupAllowLayout()
{
    auto allowLayout = new QHBoxLayout();
    allowLayout->setSpacing(20);

    m_rbAllowApp = new QRadioButton();
    m_rbAllowApp->setIcon(IconCache::icon(":/icons/accept.png"));
    m_rbAllowApp->setChecked(true);

    m_rbBlockApp = new QRadioButton();
    m_rbBlockApp->setIcon(IconCache::icon(":/icons/deny.png"));

    m_rbKillProcess = new QRadioButton();
    m_rbKillProcess->setIcon(IconCache::icon(":/icons/scull.png"));

    allowLayout->addWidget(m_rbAllowApp, 1, Qt::AlignRight);
    allowLayout->addWidget(m_rbBlockApp, 1, Qt::AlignHCenter);
    allowLayout->addWidget(m_rbKillProcess, 1, Qt::AlignLeft);

    // Block after N hours
    m_cscBlockAppIn = new CheckSpinCombo();
    m_cscBlockAppIn->spinBox()->setRange(1, 24 * 30 * 12); // ~Year
    m_cscBlockAppIn->setValues(appBlockInHourValues);
    m_cscBlockAppIn->setNamesByValues();

    // Allow Forever
    m_cbBlockAppNone = new QCheckBox();

    connect(m_rbAllowApp, &QRadioButton::toggled, this, [&](bool checked) {
        m_cbBlockAppNone->setEnabled(checked);
        m_cscBlockAppIn->setEnabled(checked);
        m_cbBlockAppAt->setEnabled(checked);
        m_dteBlockAppAt->setEnabled(checked);
    });

    connect(m_rbKillProcess, &QRadioButton::clicked, this, [&] {
        IoC<WindowManager>()->showInfoBox(
                tr("Attention: The 'Kill Process' option is very dangerous!!!\n\n"
                   "Be careful when killing a system services or other important programs!\n"
                   "It can cause a Windows malfunction or totally unusable."));
    });

    return allowLayout;
}

QLayout *ProgramEditDialog::setupCheckDateTimeEdit()
{
    m_cbBlockAppAt = new QCheckBox();

    m_dteBlockAppAt = new QDateTimeEdit();
    m_dteBlockAppAt->setCalendarPopup(true);

    return ControlUtil::createRowLayout(m_cbBlockAppAt, m_dteBlockAppAt);
}

void ProgramEditDialog::setupAllowEclusiveGroup()
{
    auto group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(m_cscBlockAppIn->checkBox());
    group->addButton(m_cbBlockAppAt);
    group->addButton(m_cbBlockAppNone);
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
        return confManager()->addApp(app);
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
    if (!app.isEqual(m_appRow)) {
        app.appId = m_appRow.appId;

        return confManager()->updateApp(app);
    }

    if (app.appName != m_appRow.appName) {
        return confManager()->updateAppName(m_appRow.appId, app.appName);
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

        if (!confManager()->updateApp(app))
            return false;
    }

    return true;
}

bool ProgramEditDialog::validateFields() const
{
    if (m_editPath->text().isEmpty()) {
        m_editPath->setFocus();
        return false;
    }

    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    return true;
}

void ProgramEditDialog::fillApp(App &app) const
{
    app.useGroupPerm = m_cbUseGroupPerm->isChecked();
    app.applyChild = m_cbApplyChild->isChecked();
    app.lanOnly = m_cbLanOnly->isChecked();
    app.logBlocked = m_cbLogBlocked->isChecked();
    app.logConn = m_cbLogConn->isChecked();
    app.blocked = !m_rbAllowApp->isChecked();
    app.killProcess = m_rbKillProcess->isChecked();
    app.groupIndex = m_comboAppGroup->currentIndex();
    app.appName = m_editName->text();

    const QString appPath = m_editPath->text();
    app.appOriginPath = appPath;
    app.appPath = FileUtil::normalizePath(appPath);

    if (!app.blocked) {
        if (m_cscBlockAppIn->checkBox()->isChecked()) {
            const int hours = m_cscBlockAppIn->spinBox()->value();

            app.endTime = QDateTime::currentDateTime().addSecs(hours * 60 * 60);
        } else if (m_cbBlockAppAt->isChecked()) {
            app.endTime = m_dteBlockAppAt->dateTime();
        }
    }
}
