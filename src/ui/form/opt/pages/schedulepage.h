#ifndef SCHEDULEPAGE_H
#define SCHEDULEPAGE_H

#include "optbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTableView)

class CheckSpinCombo;
class LabelSpin;
class LabelSpinCombo;
class TableView;
class TaskEditInfo;
class TaskInfo;
class TaskListModel;

class SchedulePage : public OptBasePage
{
    Q_OBJECT

public:
    explicit SchedulePage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    TaskListModel *taskListModel() const { return m_taskListModel; }

public slots:
    void onResetToDefault() override;

protected slots:
    void onAboutToSave() override;
    void onEditResetted() override;

    void onRetranslateUi() override;

private:
    void retranslateTaskInterval();
    void retranslateTaskRetrySeconds();

    void setupUi();
    void setupTableTasks();
    void setupTableTasksHeader();
    void setupTaskDetails();
    void setupTaskInterval();
    void setupTaskOptionsButton();
    void setupTaskStartup();
    void setupTaskMaxRetries();
    void setupTaskRetrySeconds();
    void setupTableTasksChanged();

    int currentTaskIndex() const;
    void setCurrentTaskIndex(int index);

    TaskInfo *currentTaskInfo() const { return m_taskInfo; }
    void setCurrentTaskInfo(TaskInfo *v) { m_taskInfo = v; }

    TaskEditInfo &currentTaskRow();

    void setCurrentTaskRowEdited(int role = Qt::DisplayRole);

private:
    TaskListModel *m_taskListModel = nullptr;
    TaskInfo *m_taskInfo = nullptr;

    TableView *m_tableTasks = nullptr;
    QWidget *m_taskDetailsRow = nullptr;
    CheckSpinCombo *m_cscTaskInterval = nullptr;
    QCheckBox *m_cbTaskRunOnStartup = nullptr;
    QCheckBox *m_cbTaskDelayStartup = nullptr;
    LabelSpin *m_lsTaskMaxRetries = nullptr;
    LabelSpinCombo *m_lscTaskRetrySeconds = nullptr;
    QPushButton *m_btOptions = nullptr;
    QToolButton *m_btTaskRun = nullptr;
    QToolButton *m_btTaskAbort = nullptr;
};

#endif // SCHEDULEPAGE_H
