#include "progbasepage.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/prog/programeditcontroller.h>
#include <fortmanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/ioc/ioccontainer.h>

ProgBasePage::ProgBasePage(ProgramEditController *ctrl, QWidget *parent) :
    QFrame(parent), m_ctrl(ctrl)
{
}

ConfAppManager *ProgBasePage::confAppManager() const
{
    return ctrl()->confAppManager();
}

ConfRuleManager *ProgBasePage::confRuleManager() const
{
    return ctrl()->confRuleManager();
}

ConfManager *ProgBasePage::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *ProgBasePage::conf() const
{
    return confManager()->conf();
}

IniUser *ProgBasePage::iniUser() const
{
    return &confManager()->iniUser();
}

WindowManager *ProgBasePage::windowManager() const
{
    return IoC<WindowManager>();
}

const App &ProgBasePage::app() const
{
    return ctrl()->app();
}

bool ProgBasePage::isWildcard() const
{
    return ctrl()->isWildcard();
}

bool ProgBasePage::isNew() const
{
    return ctrl()->isNew();
}

bool ProgBasePage::isSingleSelection() const
{
    return ctrl()->isSingleSelection();
}

void ProgBasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &ProgramEditController::initializePage, this, &ProgBasePage::onPageInitialize);
    connect(ctrl(), &ProgramEditController::retranslateUi, this, &ProgBasePage::onRetranslateUi);
}
