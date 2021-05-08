#ifndef SCHEDULEPAGE_H
#define SCHEDULEPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(QTableView)

class CheckSpinCombo;
class TableView;
class TaskInfo;
class TaskListModel;

class SchedulePage : public BasePage
{
    Q_OBJECT

public:
    explicit SchedulePage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    TaskListModel *taskListModel() const { return m_taskListModel; }

protected slots:
    void onAboutToSave() override;
    void onEditResetted() override;

    void onRetranslateUi() override;

private:
    void retranslateTaskDetails();

    void setupTaskListModel();

    void setupUi();
    void setupTableTasks();
    void setupTableTasksHeader();
    void setupTaskDetails();
    void setupTaskInterval();
    void setupTableTasksChanged();

    int currentTaskIndex() const;

    TaskInfo *currentTaskInfo() const { return m_taskInfo; }
    void setCurrentTaskInfo(TaskInfo *v) { m_taskInfo = v; }

private:
    TaskListModel *m_taskListModel = nullptr;
    TaskInfo *m_taskInfo = nullptr;

    TableView *m_tableTasks = nullptr;
    QWidget *m_taskDetailsRow = nullptr;
    CheckSpinCombo *m_cscTaskInterval = nullptr;
    QPushButton *m_btTaskRun = nullptr;
    QPushButton *m_btTaskAbort = nullptr;
};

#endif // SCHEDULEPAGE_H
