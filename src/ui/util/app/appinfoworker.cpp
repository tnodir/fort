#include "appinfoworker.h"

#include <QImage>

#include "appinfo.h"
#include "appinfomanager.h"
#include "apputil.h"

AppInfoWorker::AppInfoWorker(AppInfoManager *manager) :
    WorkerObject(manager)
{
}

void AppInfoWorker::doJob(const QString &appPath)
{
    AppInfo appInfo;

    AppUtil::getInfo(appPath, appInfo);

    if (aborted()) return;

    manager()->handleWorkerResult(appPath, QVariant::fromValue(appInfo));
}
