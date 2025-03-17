#include "programeditcontroller.h"

#include <conf/confappmanager.h>

ProgramEditController::ProgramEditController(QObject *parent) : BaseController(parent) { }

bool ProgramEditController::addOrUpdateApp(App &app, bool onlyUpdate)
{
    return confAppManager()->addOrUpdateApp(app, onlyUpdate);
}

bool ProgramEditController::updateApp(App &app)
{
    return confAppManager()->updateApp(app);
}

bool ProgramEditController::updateAppName(qint64 appId, const QString &appName)
{
    return confAppManager()->updateAppName(appId, appName);
}
