#include "programscontroller.h"

#include <conf/confappmanager.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/applistmodel.h>
#include <task/taskinfoapppurger.h>
#include <task/taskmanager.h>

using namespace Fort;

namespace {

void showErrorMessage(const QString &errorMessage)
{
    windowManager()->showErrorBox(errorMessage, ProgramsController::tr("App Configuration Error"));
}

}

ProgramsController::ProgramsController(QObject *parent) :
    BaseController(parent), m_appListModel(new AppListModel(this))
{
}

void ProgramsController::initialize()
{
    appListModel()->initialize();
}

void ProgramsController::updateAppsBlocked(
        const QVector<qint64> &appIdList, bool blocked, bool killProcess)
{
    if (!confAppManager()->updateAppsBlocked(appIdList, blocked, killProcess)) {
        showErrorMessage(tr("Cannot update program's state"));
    }
}

void ProgramsController::updateAppsTimer(const QVector<qint64> &appIdList, int minutes)
{
    if (!confAppManager()->updateAppsTimer(appIdList, minutes)) {
        showErrorMessage(tr("Cannot update program's state"));
    }
}

void ProgramsController::deleteApps(const QVector<qint64> &appIdList)
{
    if (!confAppManager()->deleteApps(appIdList)) {
        showErrorMessage(tr("Cannot delete program"));
    }
}

void ProgramsController::deleteAlertedApps()
{
    confAppManager()->deleteAlertedApps();
}

void ProgramsController::clearAlerts()
{
    confAppManager()->clearAlerts();
}

void ProgramsController::purgeApps()
{
    taskManager()->runTask(TaskInfo::AppPurger);
}
