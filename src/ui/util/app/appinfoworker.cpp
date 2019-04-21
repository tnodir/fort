#include "appinfoworker.h"

#include <QImage>

#include "appinfo.h"
#include "appinfomanager.h"
#include "appinfojob.h"

AppInfoWorker::AppInfoWorker(AppInfoManager *manager) :
    WorkerObject(manager)
{
}

AppInfoManager *AppInfoWorker::manager() const
{
    return static_cast<AppInfoManager *>(WorkerObject::manager());
}

void AppInfoWorker::doJob(WorkerJob *workerJob)
{
    auto job = static_cast<AppInfoJob *>(workerJob);
    const QString &appPath = job->appPath();

    if (!manager()->loadInfoFromDb(appPath, job->appInfo)
            && manager()->loadInfoFromFs(appPath, job->appInfo)) {
        const QImage appIcon = manager()->loadIconFromFs(appPath);

        manager()->saveToDb(appPath, job->appInfo, appIcon);
    }

    WorkerObject::doJob(workerJob);
}
