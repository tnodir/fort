#ifndef APPICONJOB_H
#define APPICONJOB_H

#include <QImage>

#include "appbasejob.h"

class AppIconJob : public AppBaseJob
{
public:
    explicit AppIconJob(const QString &appPath, qint64 iconId);

    virtual AppJobType jobType() const { return JobTypeIcon; }

    qint64 iconId() const { return m_iconId; }

public:
    QImage image;

private:
    const qint64 m_iconId = 0;
};

#endif // APPICONJOB_H
