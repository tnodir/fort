#include "appinfomanager.h"

#include <QImage>

#include "appinfoworker.h"

AppInfoManager::AppInfoManager(QObject *parent) :
    WorkerManager(parent)
{
    setMaxWorkersCount(1);
}

WorkerObject *AppInfoManager::createWorker()
{
    return new AppInfoWorker(this);
}

void AppInfoManager::lookupApp(const QString &appPath)
{
    enqueueJob(appPath);
}

void AppInfoManager::handleWorkerResult(const QString &appPath,
                                        const QVariant &result)
{
    auto appInfo = result.value<AppInfo>();

    emit lookupFinished(appPath, appInfo);
}
