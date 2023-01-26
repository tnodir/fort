#include "appinfoworker.h"

#include <QImage>

#include "appiconjob.h"
#include "appinfo.h"
#include "appinfojob.h"
#include "appinfomanager.h"
#include "appinfoutil.h"

AppInfoWorker::AppInfoWorker(AppInfoManager *manager) : WorkerObject(manager) { }

AppInfoManager *AppInfoWorker::manager() const
{
    return static_cast<AppInfoManager *>(WorkerObject::manager());
}

void AppInfoWorker::run()
{
    AppInfoUtil::initThread();

    WorkerObject::run();

    AppInfoUtil::doneThread();
}

void AppInfoWorker::doJob(WorkerJob *workerJob)
{
    auto appJob = static_cast<AppBaseJob *>(workerJob);
    const QString &appPath = appJob->appPath();

    switch (appJob->jobType()) {
    case AppBaseJob::JobTypeInfo: {
        auto job = static_cast<AppInfoJob *>(appJob);
        loadAppInfo(job, appPath);
    } break;
    case AppBaseJob::JobTypeIcon: {
        auto job = static_cast<AppIconJob *>(appJob);
        loadAppIcon(job, appPath);
    } break;
    }

    WorkerObject::doJob(workerJob);
}

void AppInfoWorker::loadAppInfo(AppInfoJob *job, const QString &appPath)
{
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
        const QImage appIcon = manager()->loadIconFromFs(appPath, appInfo);

        manager()->saveToDb(appPath, appInfo, appIcon);
    }
}

void AppInfoWorker::loadAppIcon(AppIconJob *job, const QString &appPath)
{
    // Try to load from DB
    job->image = manager()->loadIconFromDb(job->iconId());
}
