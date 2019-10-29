#ifndef TASKINFOTASIX_H
#define TASKINFOTASIX_H

#include "taskinfo.h"

class TaskInfoTasix : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoTasix(QObject *parent = nullptr);

public slots:
    bool processResult(FortManager *fortManager, bool success) override;
};

#endif // TASKINFOTASIX_H
