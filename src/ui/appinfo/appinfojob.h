#ifndef APPINFOJOB_H
#define APPINFOJOB_H

#include <util/worker/workerjob.h>

#include "appinfo.h"

class AppInfoJob : public WorkerJob
{
public:
    explicit AppInfoJob(const QString &appPath);

    QString appPath() const { return text; }

public:
    AppInfo appInfo;
};

#endif // APPINFOJOB_H
