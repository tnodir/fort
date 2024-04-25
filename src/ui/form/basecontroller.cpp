#include "basecontroller.h"

#include <QAbstractButton>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <conf/confzonemanager.h>
#include <conf/firewallconf.h>
#include <driver/drivermanager.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/autoupdatemanager.h>
#include <manager/hotkeymanager.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

BaseController::BaseController(QObject *parent) : QObject(parent)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &BaseController::retranslateUi);
}

FortManager *BaseController::fortManager() const
{
    return IoC<FortManager>();
}

FortSettings *BaseController::settings() const
{
    return IoC<FortSettings>();
}

ConfManager *BaseController::confManager() const
{
    return IoC<ConfManager>();
}

ConfAppManager *BaseController::confAppManager() const
{
    return IoC<ConfAppManager>();
}

ConfRuleManager *BaseController::confRuleManager() const
{
    return IoC<ConfRuleManager>();
}

ConfZoneManager *BaseController::confZoneManager() const
{
    return IoC<ConfZoneManager>();
}

FirewallConf *BaseController::conf() const
{
    return confManager()->conf();
}

IniOptions *BaseController::ini() const
{
    return &conf()->ini();
}

IniUser *BaseController::iniUser() const
{
    return &confManager()->iniUser();
}

HotKeyManager *BaseController::hotKeyManager() const
{
    return IoC<HotKeyManager>();
}

DriverManager *BaseController::driverManager() const
{
    return IoC<DriverManager>();
}

TranslationManager *BaseController::translationManager() const
{
    return IoC<TranslationManager>();
}

WindowManager *BaseController::windowManager() const
{
    return IoC<WindowManager>();
}

AutoUpdateManager *BaseController::autoUpdateManager() const
{
    return IoC<AutoUpdateManager>();
}

TaskManager *BaseController::taskManager() const
{
    return IoC<TaskManager>();
}

void BaseController::onLinkClicked()
{
    auto button = qobject_cast<QAbstractButton *>(sender());
    if (button) {
        OsUtil::openUrlOrFolder(button->windowFilePath());
    }
}
