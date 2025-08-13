#ifndef APPICONJOB_H
#define APPICONJOB_H

#include <QImage>

#include "appbasejob.h"

class AppInfoManager;

class AppIconJob : public AppBaseJob
{
public:
    explicit AppIconJob(const QString &appPath, qint64 iconId);

    qint64 iconId() const { return m_iconId; }

    void doJob(WorkerManager *manager) override;
    void reportResult(WorkerManager *manager) override;

private:
    void loadAppIcon(AppInfoManager *manager);
    void emitFinished(AppInfoManager *manager);

private:
    const qint64 m_iconId = 0;

    QImage m_image;
};

#endif // APPICONJOB_H
