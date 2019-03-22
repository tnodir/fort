#ifndef APPINFOMANAGER_H
#define APPINFOMANAGER_H

#include "../worker/workermanager.h"

class AppInfoManager : public WorkerManager
{
    Q_OBJECT

public:
    explicit AppInfoManager(QObject *parent = nullptr);

signals:
    void lookupFinished(const QString &appPath, const QString &displayName,
                        const QImage &icon);

public slots:
    void lookupApp(const QString &appPath);

    void handleWorkerResult(const QString &appPath,
                            const QVariant &result) override;

protected:
    WorkerObject *createWorker() override;
};

#endif // APPINFOMANAGER_H
