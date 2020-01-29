#ifndef TASKINFOZONEDOWNLOADER_H
#define TASKINFOZONEDOWNLOADER_H

#include "taskinfo.h"

class TaskInfoZoneDownloader : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoZoneDownloader(QObject *parent = nullptr);

public slots:
    bool processResult(FortManager *fortManager, bool success) override;
};

#endif // TASKINFOZONEDOWNLOADER_H
