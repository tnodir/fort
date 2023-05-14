#include "statbasepage.h"

#include <conf/firewallconf.h>
#include <form/stat/statisticscontroller.h>
#include <fortmanager.h>
#include <user/iniuser.h>

StatBasePage::StatBasePage(StatisticsController *ctrl, QWidget *parent) :
    QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

FortManager *StatBasePage::fortManager() const
{
    return ctrl()->fortManager();
}

ConfManager *StatBasePage::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *StatBasePage::conf() const
{
    return ctrl()->conf();
}

IniUser *StatBasePage::iniUser() const
{
    return ctrl()->iniUser();
}

TranslationManager *StatBasePage::translationManager() const
{
    return ctrl()->translationManager();
}

WindowManager *StatBasePage::windowManager() const
{
    return ctrl()->windowManager();
}

void StatBasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &StatisticsController::afterSaveWindowState, this,
            &StatBasePage::onSaveWindowState);
    connect(ctrl(), &StatisticsController::afterRestoreWindowState, this,
            &StatBasePage::onRestoreWindowState);

    connect(ctrl(), &StatisticsController::retranslateUi, this, &StatBasePage::onRetranslateUi);
}
