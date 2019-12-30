#include "appinfoworker.h"

#include <QImage>

#include "appinfo.h"
#include "appinfomanager.h"
#include "appinfojob.h"
#include "apputil.h"

AppInfoWorker::AppInfoWorker(AppInfoManager *manager) :
    WorkerObject(manager)
{
}

AppInfoManager *AppInfoWorker::manager() const
{
    return static_cast<AppInfoManager *>(WorkerObject::manager());
}

void AppInfoWorker::run()
{
    AppUtil::initThread();

    WorkerObject::run();

    AppUtil::doneThread();
}

void AppInfoWorker::doJob(WorkerJob *workerJob)
{
    auto job = static_cast<AppInfoJob *>(workerJob);
    const QString &appPath = job->appPath();

    // Try to load from DB
    AppInfo &appInfo = job->appInfo;
    bool loadedFromDb = manager()->loadInfoFromDb(appPath, appInfo);

    // Was the file modified?
    if (loadedFromDb && appInfo.isFileModified(appPath)) {
        loadedFromDb = false;
        manager()->deleteAppInfo(appPath, appInfo);
    }

    // Try to load from FS
    if (!loadedFromDb && manager()->loadInfoFromFs(appPath, appInfo)) {
        const QImage appIcon = manager()->loadIconFromFs(appPath);

        manager()->saveToDb(appPath, appInfo, appIcon);
    }

    WorkerObject::doJob(workerJob);
}
