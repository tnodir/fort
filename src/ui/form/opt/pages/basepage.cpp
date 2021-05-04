#include "basepage.h"

#include <QAbstractButton>

#include "../../../fortmanager.h"
#include "../../../util/osutil.h"
#include "../optionscontroller.h"

BasePage::BasePage(OptionsController *ctrl, QWidget *parent) : QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

FortManager *BasePage::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *BasePage::settings() const
{
    return ctrl()->settings();
}

FirewallConf *BasePage::conf() const
{
    return ctrl()->conf();
}

DriverManager *BasePage::driverManager() const
{
    return ctrl()->driverManager();
}

TranslationManager *BasePage::translationManager() const
{
    return ctrl()->translationManager();
}

TaskManager *BasePage::taskManager() const
{
    return ctrl()->taskManager();
}

ZoneListModel *BasePage::zoneListModel() const
{
    return ctrl()->zoneListModel();
}

void BasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &OptionsController::editResetted, this, &BasePage::onEditResetted);
    connect(ctrl(), &OptionsController::saved, this, &BasePage::onSaved);

    connect(ctrl(), &OptionsController::afterSaveWindowState, this, &BasePage::onSaveWindowState);
    connect(ctrl(), &OptionsController::afterRestoreWindowState, this,
            &BasePage::onRestoreWindowState);

    connect(ctrl(), &OptionsController::retranslateUi, this, &BasePage::onRetranslateUi);
}

void BasePage::onLinkClicked()
{
    auto button = qobject_cast<QAbstractButton *>(sender());
    if (button) {
        OsUtil::openUrlOrFolder(button->windowFilePath());
    }
}
