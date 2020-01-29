#ifndef TASKINFOZONEDOWNLOADER_H
#define TASKINFOZONEDOWNLOADER_H

#include "taskinfo.h"

QT_FORWARD_DECLARE_CLASS(TaskZoneDownloader)

class TaskInfoZoneDownloader : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoZoneDownloader(QObject *parent = nullptr);

    TaskZoneDownloader *zoneDownloader() const;

public slots:
    bool processResult(FortManager *fortManager, bool success) override;

protected slots:
    void setupTaskWorker() override;
};

#endif // TASKINFOZONEDOWNLOADER_H
