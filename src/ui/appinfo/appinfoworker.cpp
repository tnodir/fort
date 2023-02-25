#include "appinfoworker.h"

#include "appinfomanager.h"
#include "appinfoutil.h"

AppInfoWorker::AppInfoWorker(AppInfoManager *manager) : WorkerObject(manager) { }

void AppInfoWorker::run()
{
    AppInfoUtil::initThread();

    WorkerObject::run();

    AppInfoUtil::doneThread();
}
