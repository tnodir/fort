#ifndef APPINFOJOB_H
#define APPINFOJOB_H

#include "appbasejob.h"
#include "appinfo.h"

class AppInfoJob : public AppBaseJob
{
public:
    explicit AppInfoJob(const QString &appPath);

    virtual AppJobType jobType() const { return JobTypeInfo; }

public:
    AppInfo appInfo;
};

#endif // APPINFOJOB_H
