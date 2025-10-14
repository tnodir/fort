#include "taskinfoapppurger.h"

#include <QLoggingCategory>

#include <conf/confappmanager.h>
#include <fortglobal.h>

#include "taskmanager.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("task.appPurger");

}

TaskInfoAppPurger::TaskInfoAppPurger(TaskManager &taskManager) :
    TaskInfo(AppPurger, taskManager) { }

void TaskInfoAppPurger::runTaskWorker()
{
    // TODO: Use worker to run in a separate thread to not block the UI
    const bool ok = confAppManager()->purgeApps();

    qCDebug(LC) << "Purged:" << ok;

    handleFinished(ok);
}
