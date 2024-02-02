#ifndef TASKINFOAPPPURGER_H
#define TASKINFOAPPPURGER_H

#include "taskinfo.h"

class TaskAppPurger;

class TaskInfoAppPurger : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoAppPurger(TaskManager &taskManager);

public slots:
    bool processResult(bool success) override { return success; }

protected slots:
    void setupTaskWorker() override { }
    void runTaskWorker() override;
};

#endif // TASKINFOAPPPURGER_H
