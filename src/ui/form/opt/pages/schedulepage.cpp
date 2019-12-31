#include "schedulepage.h"

#include <QCheckBox>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QVBoxLayout>

#include "../../../task/taskinfo.h"
#include "../../../task/tasklistmodel.h"
#include "../../../task/taskmanager.h"
#include "../../controls/checkspincombo.h"
#include "../../controls/controlutil.h"
#include "../../controls/tableview.h"
#include "../optionscontroller.h"

namespace {

const ValuesList taskIntervalHourValues = {
    3, 1, 6, 12, 24, 24 * 7, 24 * 30
};

}

SchedulePage::SchedulePage(OptionsController *ctrl,
                           QWidget *parent) :
    BasePage(ctrl, parent),
    m_taskListModel(new TaskListModel(taskManager(), this))
{
    setupTaskListModel();

    setupUi();
}

void SchedulePage::setScheduleEdited(bool v)
{
    if (m_scheduleEdited != v) {
        m_scheduleEdited = v;

        if (scheduleEdited()) {
            ctrl()->setOthersEdited(true);
        }
    }
}

void SchedulePage::onEditResetted()
{
    setScheduleEdited(false);

    m_taskListModel->resetEdited();
}

void SchedulePage::onSaved()
{
    if (!scheduleEdited()) return;

    m_taskListModel->saveChanges();
}

void SchedulePage::onRetranslateUi()
{
    m_taskListModel->refresh();

    m_btTaskRun->setText(tr("Run"));
    m_btTaskAbort->setText(tr("Abort"));

    retranslateTaskDetails();
}

void SchedulePage::setupTaskListModel()
{
    connect(m_taskListModel, &TaskListModel::dataChanged, [&] {
        setScheduleEdited(true);
    });
}

void SchedulePage::retranslateTaskDetails()
{
    const QStringList list = {
        tr("Custom"), tr("Hourly"), tr("Each 6 hours"),
        tr("Each 12 hours"), tr("Daily"), tr("Weekly"), tr("Monthly")
    };

    m_lscTaskInterval->setNames(list);
    m_lscTaskInterval->spinBox()->setSuffix(tr(" hours"));
}

void SchedulePage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Tasks Table
    setupTableTasks();
    setupTableTasksHeader();
    layout->addWidget(m_tableTasks, 1);

    // Task Details
    setupTaskDetails();
    layout->addWidget(m_taskDetailsRow);

    // Actions on tasks table's current changed
    setupTableTasksChanged();

    this->setLayout(layout);
}

void SchedulePage::setupTableTasks()
{
    m_tableTasks = new TableView();
    m_tableTasks->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableTasks->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_tableTasks->setModel(taskListModel());
}

void SchedulePage::setupTableTasksHeader()
{
    auto header = m_tableTasks->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);

    const auto refreshTableTasksHeader = [&] {
        auto hh = m_tableTasks->horizontalHeader();
        hh->resizeSection(0, qRound(hh->width() * 0.45));
    };

    refreshTableTasksHeader();

    connect(header, &QHeaderView::geometriesChanged, this, refreshTableTasksHeader);
}

void SchedulePage::setupTaskDetails()
{
    m_taskDetailsRow = new QWidget();

    auto layout = new QHBoxLayout();
    layout->setMargin(0);
    m_taskDetailsRow->setLayout(layout);

    setupTaskInterval();

    m_btTaskRun = ControlUtil::createButton(":/images/run.png", [&] {
        currentTaskInfo()->run();
    });
    m_btTaskAbort = ControlUtil::createButton(":/images/cancel.png", [&] {
        currentTaskInfo()->abort();
    });

    layout->addWidget(m_lscTaskInterval, 1);
    layout->addWidget(m_btTaskRun);
    layout->addWidget(m_btTaskAbort);
}

void SchedulePage::setupTaskInterval()
{
    m_lscTaskInterval = new CheckSpinCombo();
    m_lscTaskInterval->checkBox()->setFont(ControlUtil::fontDemiBold());
    m_lscTaskInterval->spinBox()->setRange(1, 24 * 30 * 12);  // ~Year
    m_lscTaskInterval->setValues(taskIntervalHourValues);

    connect(m_lscTaskInterval->checkBox(), &QCheckBox::toggled, [&](bool checked) {
        const int taskIndex = currentTaskIndex();
        const auto index = taskListModel()->index(taskIndex, 0);

        taskListModel()->setData(index, checked, TaskListModel::RoleEnabled);
    });
    connect(m_lscTaskInterval->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
        const int taskIndex = currentTaskIndex();
        const auto index = taskListModel()->index(taskIndex, 1);

        taskListModel()->setData(index, value, TaskListModel::RoleIntervalHours);
    });
}

void SchedulePage::setupTableTasksChanged()
{
    const auto refreshTableTasksChanged = [&] {
        const int taskIndex = currentTaskIndex();
        const bool taskSelected = taskIndex >= 0;

        setCurrentTaskInfo(taskSelected
                           ? taskListModel()->taskInfoAt(taskIndex)
                           : nullptr);
        m_taskDetailsRow->setVisible(taskSelected);

        if (taskSelected) {
            const auto index = taskListModel()->index(taskIndex, 0);

            m_lscTaskInterval->checkBox()->setChecked(
                        taskListModel()->data(index, TaskListModel::RoleEnabled).toBool());
            m_lscTaskInterval->checkBox()->setText(
                        taskListModel()->data(index).toString());
            m_lscTaskInterval->spinBox()->setValue(
                        taskListModel()->data(index, TaskListModel::RoleIntervalHours).toInt());

            const bool running = currentTaskInfo()->running();
            m_btTaskRun->setEnabled(!running);
            m_btTaskAbort->setEnabled(running);
        }
    };

    refreshTableTasksChanged();

    connect(m_tableTasks, &TableView::currentIndexChanged, this, refreshTableTasksChanged);
    connect(taskListModel(), &TaskListModel::dataChanged, this, refreshTableTasksChanged);
    connect(taskManager(), &TaskManager::taskStarted, this, refreshTableTasksChanged);
    connect(taskManager(), &TaskManager::taskFinished, this, refreshTableTasksChanged);
}

int SchedulePage::currentTaskIndex() const
{
    return m_tableTasks->currentIndex().row();
}
