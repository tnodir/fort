#include "schedulepage.h"

#include <QCheckBox>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSpinBox>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <form/controls/checkspincombo.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/opt/optionscontroller.h>
#include <task/taskinfo.h>
#include <task/tasklistmodel.h>
#include <task/taskmanager.h>
#include <util/guiutil.h>

namespace {

const std::array taskIntervalHourValues = { 3, 1, 6, 12, 24, 24 * 7, 24 * 30 };

}

SchedulePage::SchedulePage(OptionsController *ctrl, QWidget *parent) :
    OptBasePage(ctrl, parent), m_taskListModel(new TaskListModel(taskManager(), this))
{
    setupUi();
}

void SchedulePage::onAboutToSave()
{
    if (conf()->taskEdited()) {
        ini()->setTaskInfoList(taskListModel()->toVariant());
    }
}

void SchedulePage::onEditResetted()
{
    taskListModel()->setupTaskRows();
}

void SchedulePage::onRetranslateUi()
{
    taskListModel()->refresh();

    retranslateTaskInterval();

    m_cbTaskRunOnStartup->setText(tr("Run On Startup"));

    m_btTaskRun->setText(tr("Run"));
    m_btTaskAbort->setText(tr("Abort"));
}

void SchedulePage::retranslateTaskInterval()
{
    const QStringList list = { tr("Custom"), tr("Hourly"), tr("Each 6 hours"), tr("Each 12 hours"),
        tr("Daily"), tr("Weekly"), tr("Monthly") };

    m_cscTaskInterval->setNames(list);
    m_cscTaskInterval->spinBox()->setSuffix(tr(" hour(s)"));
}

void SchedulePage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Task Details
    setupTaskDetails();

    // Tasks Table
    setupTableTasks();
    setupTableTasksHeader();

    // Actions on tasks table's current changed
    setupTableTasksChanged();

    // Select the task
    setCurrentTaskIndex(0);

    layout->addWidget(m_taskDetailsRow);
    layout->addWidget(m_tableTasks, 1);

    this->setLayout(layout);
}

void SchedulePage::setupTableTasks()
{
    m_tableTasks = new TableView();
    m_tableTasks->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableTasks->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_tableTasks->setModel(taskListModel());

    connect(m_tableTasks, &TableView::doubleClicked, this, [&](const QModelIndex &index) {
        const auto taskInfo = taskListModel()->taskInfoAt(index.row());
        emit taskManager()->taskDoubleClicked(taskInfo->type());
    });
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
    setupTaskInterval();
    setupTaskRunOnStartup();

    m_btTaskRun = ControlUtil::createFlatToolButton(
            ":/icons/play.png", [&] { taskManager()->runTask(currentTaskInfo()->type()); });
    m_btTaskAbort = ControlUtil::createFlatToolButton(
            ":/icons/cancel.png", [&] { taskManager()->abortTask(currentTaskInfo()->type()); });

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_cscTaskInterval, 1);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_cbTaskRunOnStartup);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btTaskRun);
    layout->addWidget(m_btTaskAbort);

    m_taskDetailsRow = new QWidget();
    m_taskDetailsRow->setLayout(layout);
}

void SchedulePage::setupTaskInterval()
{
    m_cscTaskInterval = new CheckSpinCombo();
    m_cscTaskInterval->checkBox()->setFont(GuiUtil::fontBold());
    m_cscTaskInterval->spinBox()->setRange(1, 24 * 30 * 12); // ~Year
    m_cscTaskInterval->setValues(taskIntervalHourValues);

    connect(m_cscTaskInterval->checkBox(), &QCheckBox::toggled, this, [&](bool checked) {
        const int taskIndex = currentTaskIndex();
        if (taskIndex >= 0) {
            const auto index = taskListModel()->index(taskIndex, 0);
            taskListModel()->setData(index, checked, TaskListModel::RoleEnabled);
        }
    });
    connect(m_cscTaskInterval->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                const int taskIndex = currentTaskIndex();
                if (taskIndex >= 0) {
                    const auto index = taskListModel()->index(taskIndex, 1);
                    taskListModel()->setData(index, value, TaskListModel::RoleIntervalHours);
                }
            });
}

void SchedulePage::setupTaskRunOnStartup()
{
    m_cbTaskRunOnStartup = ControlUtil::createCheckBox(false, [&](bool checked) {
        const int taskIndex = currentTaskIndex();
        if (taskIndex >= 0) {
            const auto index = taskListModel()->index(taskIndex, 2);
            taskListModel()->setData(index, checked, TaskListModel::RoleRunOnStartup);
        }
    });
}

void SchedulePage::setupTableTasksChanged()
{
    const auto refreshTableTasksChanged = [&] {
        const int taskIndex = currentTaskIndex();
        const bool taskSelected = (taskIndex >= 0);

        setCurrentTaskInfo(taskSelected ? taskListModel()->taskInfoAt(taskIndex) : nullptr);
        m_taskDetailsRow->setEnabled(taskSelected);

        if (taskSelected) {
            const auto index = taskListModel()->index(taskIndex, 0);

            m_cscTaskInterval->checkBox()->setChecked(
                    taskListModel()->data(index, TaskListModel::RoleEnabled).toBool());
            m_cscTaskInterval->checkBox()->setText(taskListModel()->data(index).toString());
            m_cscTaskInterval->spinBox()->setValue(
                    taskListModel()->data(index, TaskListModel::RoleIntervalHours).toInt());

            m_cbTaskRunOnStartup->setChecked(
                    taskListModel()->data(index, TaskListModel::RoleRunOnStartup).toBool());

            const bool running = currentTaskInfo()->running();
            m_btTaskRun->setEnabled(!running);
            m_btTaskAbort->setEnabled(running);
        } else {
            m_cscTaskInterval->checkBox()->setText(QString());
        }
    };

    refreshTableTasksChanged();

    connect(m_tableTasks, &TableView::currentIndexChanged, this, refreshTableTasksChanged);
    connect(taskListModel(), &TaskListModel::dataChanged, this, refreshTableTasksChanged);

    connect(taskListModel(), &TaskListModel::dataEdited, ctrl(), &OptionsController::setTaskEdited);
}

int SchedulePage::currentTaskIndex() const
{
    return m_tableTasks->currentRow();
}

void SchedulePage::setCurrentTaskIndex(int index)
{
    m_tableTasks->selectCell(index);
}
