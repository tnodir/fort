#ifndef APPBASEJOB_H
#define APPBASEJOB_H

#include <util/worker/workerjob.h>

class AppBaseJob : public WorkerJob
{
public:
    explicit AppBaseJob(const QString &appPath);

    const QString &appPath() const { return text(); }
};

#endif // APPBASEJOB_H
