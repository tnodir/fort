#ifndef SCHEDULEPAGE_H
#define SCHEDULEPAGE_H

#include "optbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTableView)

class CheckSpinCombo;
class TableView;
class TaskInfo;
class TaskListModel;

class SchedulePage : public OptBasePage
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
    void retranslateTaskInterval();

    void setupUi();
    void setupTableTasks();
    void setupTableTasksHeader();
    void setupTaskDetails();
    void setupTaskInterval();
    void setupTaskRunOnStartup();
    void setupTableTasksChanged();

    int currentTaskIndex() const;
    void setCurrentTaskIndex(int index);

    TaskInfo *currentTaskInfo() const { return m_taskInfo; }
    void setCurrentTaskInfo(TaskInfo *v) { m_taskInfo = v; }

private:
    TaskListModel *m_taskListModel = nullptr;
    TaskInfo *m_taskInfo = nullptr;

    TableView *m_tableTasks = nullptr;
    QWidget *m_taskDetailsRow = nullptr;
    CheckSpinCombo *m_cscTaskInterval = nullptr;
    QCheckBox *m_cbTaskRunOnStartup = nullptr;
    QToolButton *m_btTaskRun = nullptr;
    QToolButton *m_btTaskAbort = nullptr;
};

#endif // SCHEDULEPAGE_H
