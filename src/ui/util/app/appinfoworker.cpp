#include "appinfoworker.h"

#include <QImage>

#include "appinfomanager.h"

AppInfoWorker::AppInfoWorker(AppInfoManager *manager) :
    WorkerObject(manager)
{
}

void AppInfoWorker::doJob(const QString &appPath)
{
    // TODO

    if (aborted()) return;

    const QVariantList result = QVariantList()
            << QString() << QImage();

    manager()->handleWorkerResult(appPath, result);
}
