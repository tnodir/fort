#include "appinfojob.h"

#include <QImage>

#include <util/worker/workerobject.h>

#include "appinfo.h"
#include "appinfomanager.h"

AppInfoJob::AppInfoJob(const QString &appPath) : AppBaseJob(appPath) { }

void AppInfoJob::doJob(WorkerManager *manager)
{
    loadAppInfo(static_cast<AppInfoManager *>(manager));
}

void AppInfoJob::reportResult(WorkerManager *manager)
{
    emitFinished(static_cast<AppInfoManager *>(manager));
}

void AppInfoJob::loadAppInfo(AppInfoManager *manager)
{
    // Try to load from DB
    bool loadedFromDb = manager->loadInfoFromDb(appPath(), m_appInfo);

    // Was the file modified?
    if (loadedFromDb && m_appInfo.checkFileModified(appPath())) {
        loadedFromDb = false;
        manager->deleteAppInfo(appPath(), m_appInfo);
    }

    // Try to load from FS
    if (!loadedFromDb && manager->loadInfoFromFs(appPath(), m_appInfo)) {
        const QImage appIcon = manager->loadIconFromFs(appPath(), m_appInfo);

        manager->saveToDb(appPath(), m_appInfo, appIcon);
    }
}

void AppInfoJob::emitFinished(AppInfoManager *manager)
{
    emit manager->lookupInfoFinished(appPath(), m_appInfo);
}
