#ifndef APPBASEJOB_H
#define APPBASEJOB_H

#include <util/worker/workerjob.h>

class AppBaseJob : public WorkerJob
{
public:
    enum AppJobType : qint8 { JobTypeInfo, JobTypeIcon };

    explicit AppBaseJob(const QString &appPath);

    virtual AppJobType jobType() const = 0;

    const QString &appPath() const { return text; }
};

#endif // APPBASEJOB_H
