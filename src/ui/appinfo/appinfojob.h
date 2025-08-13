#ifndef APPINFOJOB_H
#define APPINFOJOB_H

#include "appbasejob.h"
#include "appinfo.h"

class AppInfoManager;

class AppInfoJob : public AppBaseJob
{
public:
    explicit AppInfoJob(const QString &appPath);

    void doJob(WorkerManager *manager) override;
    void reportResult(WorkerManager *manager) override;

private:
    void loadAppInfo(AppInfoManager *manager);
    void emitFinished(AppInfoManager *manager);

private:
    AppInfo m_appInfo;
};

#endif // APPINFOJOB_H
