#include "schedulepage.h"

#include <QCheckBox>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <form/controls/checkspincombo.h>
#include <form/controls/controlutil.h>
#include <form/controls/labelspin.h>
#include <form/controls/labelspincombo.h>
#include <form/controls/tableview.h>
#include <form/opt/optionscontroller.h>
#include <task/taskinfo.h>
#include <task/tasklistmodel.h>
#include <task/taskmanager.h>
#include <util/guiutil.h>

namespace {

const std::array taskIntervalHourValues = { 3, 1, 6, 12, 24, 24 * 7, 24 * 30 };
const std::array taskRetrySecondsValues = { 3, 1, 20, 60, 2 * 60, 5 * 60, 10 * 60 };

}

SchedulePage::SchedulePage(OptionsController *ctrl, QWidget *parent) :
    OptBasePage(ctrl, parent), m_taskListModel(new TaskListModel(taskManager(), this))
{
    setupUi();
}

void SchedulePage::onResetToDefault()
{
    auto model = taskListModel();

    const int n = model->rowCount();
    for (int i = 0; i < n; ++i) {
        auto &task = model->taskRowAt(i);

        task.resetToDefault();
        model->setTaskRowEdited(i);
    }
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

    m_btOptions->setText(tr("Options"));

    m_cbTaskRunOnStartup->setText(tr("Run On Startup"));
    m_cbTaskDelayStartup->setText(tr("Delay startup to retry's seconds"));

    m_lsTaskMaxRetries->label()->setText(tr("Maximum retries count"));
    m_lscTaskRetrySeconds->label()->setText(tr("Delay seconds to retry"));
    retranslateTaskRetrySeconds();

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

void SchedulePage::retranslateTaskRetrySeconds()
{
    const QStringList list = { tr("Custom"), tr("3 seconds"), tr("20 seconds"), tr("1 minute"),
        tr("2 minutes"), tr("5 minutes"), tr("10 minutes") };

    m_lscTaskRetrySeconds->setNames(list);
    m_lscTaskRetrySeconds->spinBox()->setSuffix(tr(" second(s)"));
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
        emit taskManager() -> taskDoubleClicked(taskInfo->type());
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
    setupTaskOptionsButton();

    m_btTaskRun = ControlUtil::createFlatToolButton(
            ":/icons/play.png", [&] { taskManager()->runTask(currentTaskInfo()->type()); });
    m_btTaskAbort = ControlUtil::createFlatToolButton(
            ":/icons/cancel.png", [&] { taskManager()->abortTask(currentTaskInfo()->type()); });

    auto layout = ControlUtil::createHLayout();
    layout->addWidget(m_cscTaskInterval, 1);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btOptions);
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
        auto &task = currentTaskRow();

        task.setEnabled(checked);
        setCurrentTaskRowEdited(Qt::CheckStateRole);
    });

    connect(m_cscTaskInterval->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                auto &task = currentTaskRow();

                task.setIntervalHours(value);
                setCurrentTaskRowEdited();
            });
}

void SchedulePage::setupTaskOptionsButton()
{
    setupTaskStartup();
    setupTaskMaxRetries();
    setupTaskRetrySeconds();

    // Menu
    auto layout = ControlUtil::createVLayoutByWidgets({ m_cbTaskRunOnStartup, m_cbTaskDelayStartup,
            ControlUtil::createHSeparator(), m_lsTaskMaxRetries, m_lscTaskRetrySeconds });

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btOptions = ControlUtil::createButton(":/icons/widgets.png");
    m_btOptions->setMenu(menu);
}

void SchedulePage::setupTaskStartup()
{
    m_cbTaskRunOnStartup = ControlUtil::createCheckBox(false, [&](bool checked) {
        auto &task = currentTaskRow();

        task.setRunOnStartup(checked);
        setCurrentTaskRowEdited();
    });

    m_cbTaskDelayStartup = ControlUtil::createCheckBox(false, [&](bool checked) {
        auto &task = currentTaskRow();

        task.setDelayStartup(checked);
        setCurrentTaskRowEdited();
    });
}

void SchedulePage::setupTaskMaxRetries()
{
    m_lsTaskMaxRetries = new LabelSpin();
    m_lsTaskMaxRetries->spinBox()->setRange(0, 99);

    connect(m_lsTaskMaxRetries->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                auto &task = currentTaskRow();

                task.setMaxRetries(value);
                setCurrentTaskRowEdited();
            });
}

void SchedulePage::setupTaskRetrySeconds()
{
    m_lscTaskRetrySeconds = new LabelSpinCombo();
    m_lscTaskRetrySeconds->spinBox()->setRange(0, 60000); // ~16.6 hours
    m_lscTaskRetrySeconds->setValues(taskRetrySecondsValues);

    connect(m_lscTaskRetrySeconds->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                auto &task = currentTaskRow();

                task.setRetrySeconds(value);
                setCurrentTaskRowEdited();
            });
}

void SchedulePage::setupTableTasksChanged()
{
    const auto refreshTableTasksChanged = [&] {
        const int taskIndex = currentTaskIndex();
        const bool taskSelected = (taskIndex >= 0);

        const auto taskInfo = taskSelected ? taskListModel()->taskInfoAt(taskIndex) : nullptr;
        setCurrentTaskInfo(taskInfo);

        m_taskDetailsRow->setEnabled(taskSelected);

        if (taskSelected) {
            const auto &task = taskListModel()->taskRowAt(taskIndex);

            m_cscTaskInterval->checkBox()->setChecked(task.enabled());
            m_cscTaskInterval->checkBox()->setText(taskInfo->title());
            m_cscTaskInterval->spinBox()->setValue(task.intervalHours());

            m_cbTaskRunOnStartup->setChecked(task.runOnStartup());
            m_cbTaskDelayStartup->setChecked(task.delayStartup());

            const bool running = taskInfo->running();
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

TaskEditInfo &SchedulePage::currentTaskRow()
{
    return taskListModel()->taskRowAt(currentTaskIndex());
}

void SchedulePage::setCurrentTaskRowEdited(int role)
{
    taskListModel()->setTaskRowEdited(currentTaskIndex(), role);
}
