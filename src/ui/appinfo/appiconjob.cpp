#include "appiconjob.h"

#include <util/worker/workerobject.h>

#include "appinfomanager.h"

AppIconJob::AppIconJob(const QString &appPath, qint64 iconId) :
    AppBaseJob(appPath), m_iconId(iconId)
{
}

void AppIconJob::doJob(WorkerManager *manager)
{
    loadAppIcon(static_cast<AppInfoManager *>(manager));
}

void AppIconJob::reportResult(WorkerManager *manager)
{
    emitFinished(static_cast<AppInfoManager *>(manager));
}

void AppIconJob::loadAppIcon(AppInfoManager *manager)
{
    // Try to load from DB
    m_image = manager->loadIconFromDb(iconId());
}

void AppIconJob::emitFinished(AppInfoManager *manager)
{
    emit manager->lookupIconFinished(appPath(), m_image);
}
