#include "taskinfoapppurger.h"

#include <conf/confappmanager.h>
#include <util/ioc/ioccontainer.h>

#include "taskmanager.h"

TaskInfoAppPurger::TaskInfoAppPurger(TaskManager &taskManager) :
    TaskInfo(AppPurger, taskManager) { }

void TaskInfoAppPurger::runTaskWorker()
{
    // TODO: Use worker to run in a separate thread to not block the UI
    const bool ok = IoC<ConfAppManager>()->purgeApps();

    handleFinished(ok);
}
