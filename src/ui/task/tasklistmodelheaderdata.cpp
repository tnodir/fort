#include "tasklistmodelheaderdata.h"

#include <QFont>

#include <util/iconcache.h>

#include "tasklistmodel.h"

namespace {

QVariant headerDataDisplayName(int /*role*/)
{
    return TaskListModel::tr("Name");
}

QVariant headerDataDisplayInterval(int /*role*/)
{
    return TaskListModel::tr("Interval, hours");
}

QVariant headerDataDisplayStartup(int role)
{
    return (role == Qt::ToolTipRole) ? TaskListModel::tr("Run On Startup") : QString();
}

QVariant headerDataDisplayRetry(int role)
{
    return (role == Qt::ToolTipRole) ? TaskListModel::tr("Maximum retries count") : QString();
}

QVariant headerDataDisplayLastRun(int /*role*/)
{
    return TaskListModel::tr("Last Run");
}

QVariant headerDataDisplayLastSuccess(int /*role*/)
{
    return TaskListModel::tr("Last Success");
}

using headerDataDisplay_func = QVariant (*)(int role);

static const headerDataDisplay_func headerDataDisplay_funcList[] = {
    &headerDataDisplayName,
    &headerDataDisplayInterval,
    &headerDataDisplayStartup,
    &headerDataDisplayRetry,
    &headerDataDisplayLastRun,
    &headerDataDisplayLastSuccess,
};

}

TaskListModelHeaderData::TaskListModelHeaderData(int column, int role) :
    m_column(column), m_role(role)
{
}

QVariant TaskListModelHeaderData::headerDataDisplay() const
{
    const headerDataDisplay_func func = headerDataDisplay_funcList[column()];

    return func(role());
}

QVariant TaskListModelHeaderData::headerDataDecoration() const
{
    switch (column()) {
    case 2:
        return IconCache::icon(":/icons/play.png");
    case 3:
        return IconCache::icon(":/icons/arrow_rotate_clockwise.png");
    }

    return {};
}
