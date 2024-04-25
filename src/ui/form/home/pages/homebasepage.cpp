#include "homebasepage.h"

#include <conf/firewallconf.h>
#include <form/home/homecontroller.h>
#include <fortmanager.h>
#include <user/iniuser.h>

HomeBasePage::HomeBasePage(HomeController *ctrl, QWidget *parent) : QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

FortManager *HomeBasePage::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *HomeBasePage::settings() const
{
    return ctrl()->settings();
}

ConfManager *HomeBasePage::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *HomeBasePage::conf() const
{
    return ctrl()->conf();
}

IniUser *HomeBasePage::iniUser() const
{
    return ctrl()->iniUser();
}

DriverManager *HomeBasePage::driverManager() const
{
    return ctrl()->driverManager();
}

AutoUpdateManager *HomeBasePage::autoUpdateManager() const
{
    return ctrl()->autoUpdateManager();
}

TaskManager *HomeBasePage::taskManager() const
{
    return ctrl()->taskManager();
}

TranslationManager *HomeBasePage::translationManager() const
{
    return ctrl()->translationManager();
}

WindowManager *HomeBasePage::windowManager() const
{
    return ctrl()->windowManager();
}

void HomeBasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &HomeController::afterSaveWindowState, this, &HomeBasePage::onSaveWindowState);
    connect(ctrl(), &HomeController::afterRestoreWindowState, this,
            &HomeBasePage::onRestoreWindowState);

    connect(ctrl(), &HomeController::retranslateUi, this, &HomeBasePage::onRetranslateUi);
    connect(ctrl(), &HomeController::passwordLockedChanged, this,
            &HomeBasePage::onPasswordLockedChanged);

    onPasswordLockedChanged();
}

void HomeBasePage::onPasswordLockedChanged()
{
    setEnabled(!ctrl()->passwordLocked());
}
