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

#include "../../conf/confmanager.h"
#include "../../fortmanager.h"
#include "../../util/app/appinfocache.h"
#include "../../util/iconcache.h"
#include "../controls/checkspincombo.h"
#include "../controls/controlutil.h"
#include "programscontroller.h"

namespace {

const ValuesList appBlockInHourValues = { 3, 1, 6, 12, 24, 24 * 7, 24 * 30 };

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

AppListModel *ProgramEditDialog::appListModel() const
{
    return fortManager()->appListModel();
}

AppInfoCache *ProgramEditDialog::appInfoCache() const
{
    return appListModel()->appInfoCache();
}

void ProgramEditDialog::initialize(const AppRow &appRow, const QVector<qint64> &appIdList)
{
    m_appRow = appRow;
    m_appIdList = appIdList;

    const bool isSingleSelection = (appIdList.size() <= 1);
    const bool isPathEditable = isSingleSelection && appRow.appId == 0;

    m_editPath->setText(isSingleSelection ? appRow.appPath : "*");
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
    m_rbAllowApp->setChecked(!appRow.blocked);
    m_rbBlockApp->setChecked(appRow.blocked);
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
    this->raise();
    this->activateWindow();

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

    m_labelEditPath->setText(tr("Program Path:"));
    m_btSelectFile->setToolTip(tr("Select File"));
    m_labelEditName->setText(tr("Program Name:"));
    m_btGetName->setToolTip(tr("Get Program Name"));

    m_labelAppGroup->setText(tr("Application Group:"));
    m_cbUseGroupPerm->setText(tr("Use Application Group's Enabled State"));
    m_rbAllowApp->setText(tr("Allow"));
    m_rbBlockApp->setText(tr("Block"));

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
    layout->addLayout(allowLayout);
    layout->addWidget(m_cscBlockAppIn);
    layout->addLayout(blockAtLayout);
    layout->addWidget(m_cbBlockAppNone);
    layout->addStretch();
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

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

    return layout;
}

QLayout *ProgramEditDialog::setupAppPathLayout()
{
    auto layout = new QHBoxLayout();

    m_editPath = new QLineEdit();

    m_btSelectFile = ControlUtil::createFlatButton(":/icons/folder-open.png", [&] {
        const auto filePath = ControlUtil::getOpenFileName(
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

        const QString appName = appInfoCache()->appName(appPath);
        m_editName->setText(appName);
    };

    m_btGetName = ControlUtil::createFlatButton(":/icons/sign-sync.png", updateAppName);

    layout->addWidget(m_editName);
    layout->addWidget(m_btGetName);

    return layout;
}

void ProgramEditDialog::setupComboAppGroups()
{
    m_comboAppGroup = new QComboBox();

    const auto refreshComboAppGroups = [&](bool onlyFlags = false) {
        if (onlyFlags)
            return;

        m_comboAppGroup->clear();
        m_comboAppGroup->addItems(appListModel()->appGroupNames());
        m_comboAppGroup->setCurrentIndex(0);
    };

    refreshComboAppGroups();

    connect(confManager(), &ConfManager::confSaved, this, refreshComboAppGroups);
}

QLayout *ProgramEditDialog::setupAllowLayout()
{
    auto allowLayout = new QHBoxLayout();
    allowLayout->setSpacing(20);

    m_rbAllowApp = new QRadioButton();
    m_rbAllowApp->setIcon(IconCache::icon(":/icons/sign-check.png"));
    m_rbAllowApp->setChecked(true);

    m_rbBlockApp = new QRadioButton();
    m_rbBlockApp->setIcon(IconCache::icon(":/icons/sign-ban.png"));

    allowLayout->addWidget(m_rbAllowApp, 1, Qt::AlignRight);
    allowLayout->addWidget(m_rbBlockApp, 1, Qt::AlignLeft);

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

    const QString appPath = m_editPath->text();
    if (isSingleSelection && appPath.isEmpty()) {
        m_editPath->setFocus();
        return false;
    }

    const QString appName = m_editName->text();
    if (isSingleSelection && appName.isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    const int groupIndex = m_comboAppGroup->currentIndex();
    const bool useGroupPerm = m_cbUseGroupPerm->isChecked();
    const bool blocked = m_rbBlockApp->isChecked();

    QDateTime endTime;
    if (!blocked) {
        if (m_cscBlockAppIn->checkBox()->isChecked()) {
            const int hours = m_cscBlockAppIn->spinBox()->value();

            endTime = QDateTime::currentDateTime().addSecs(hours * 60 * 60);
        } else if (m_cbBlockAppAt->isChecked()) {
            endTime = m_dteBlockAppAt->dateTime();
        }
    }

    // Add new app or edit non-selected app
    if (appIdsCount == 0) {
        return appListModel()->addApp(appPath, appName, endTime, groupIndex, useGroupPerm, blocked);
    }

    // Edit selected app
    if (isSingleSelection) {
        return saveApp(appPath, appName, endTime, groupIndex, useGroupPerm, blocked);
    }

    // Edit selected apps
    return saveMulti(endTime, groupIndex, useGroupPerm, blocked);
}

bool ProgramEditDialog::saveApp(const QString &appPath, const QString &appName,
        const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool blocked)
{
    const bool appEdited = (appPath != m_appRow.appPath || groupIndex != m_appRow.groupIndex
            || useGroupPerm != m_appRow.useGroupPerm || blocked != m_appRow.blocked
            || endTime != m_appRow.endTime);

    if (appEdited) {
        return appListModel()->updateApp(
                m_appRow.appId, appPath, appName, endTime, groupIndex, useGroupPerm, blocked);
    }

    if (appName == m_appRow.appName)
        return true;

    return appListModel()->updateAppName(m_appRow.appId, appName);
}

bool ProgramEditDialog::saveMulti(
        const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool blocked)
{
    for (qint64 appId : m_appIdList) {
        const auto appRow = appListModel()->appRowById(appId);

        if (!appListModel()->updateApp(appId, appRow.appPath, appRow.appName, endTime, groupIndex,
                    useGroupPerm, blocked))
            return false;
    }

    return true;
}
