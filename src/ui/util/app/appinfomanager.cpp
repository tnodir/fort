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
    const QVariantList list = result.toList();
    const QString &displayName = list.at(0).toString();
    const QImage &icon = list.at(1).value<QImage>();

    emit lookupFinished(appPath, displayName, icon);
}
