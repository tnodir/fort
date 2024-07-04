#include "applistmodelheaderdata.h"

#include <QFont>
#include <QIcon>

#include <util/iconcache.h>

#include "applistmodel.h"

namespace {

QVariant headerDataDisplayName(int /*role*/)
{
    return AppListModel::tr("Name");
}

QVariant headerDataDisplayZones(int role)
{
    return (role == Qt::ToolTipRole) ? AppListModel::tr("Zones") : QString();
}

QVariant headerDataDisplayRule(int role)
{
    return (role == Qt::ToolTipRole) ? AppListModel::tr("Rule") : QString();
}

QVariant headerDataDisplayScheduled(int role)
{
    return (role == Qt::ToolTipRole) ? AppListModel::tr("Scheduled") : QString();
}

QVariant headerDataDisplayAction(int /*role*/)
{
    return AppListModel::tr("Action");
}

QVariant headerDataDisplayGroup(int /*role*/)
{
    return AppListModel::tr("Group");
}

QVariant headerDataDisplayFilePath(int /*role*/)
{
    return AppListModel::tr("File Path");
}

QVariant headerDataDisplayCreationTime(int /*role*/)
{
    return AppListModel::tr("Creation Time");
}

using headerDataDisplay_func = QVariant (*)(int role);

static const headerDataDisplay_func headerDataDisplay_funcList[] = {
    &headerDataDisplayName,
    &headerDataDisplayZones,
    &headerDataDisplayRule,
    &headerDataDisplayScheduled,
    &headerDataDisplayAction,
    &headerDataDisplayGroup,
    &headerDataDisplayFilePath,
    &headerDataDisplayCreationTime,
};

}

AppListModelHeaderData::AppListModelHeaderData(int column, int role) :
    m_column(column), m_role(role)
{
}

QVariant AppListModelHeaderData::headerDataDisplay() const
{
    const headerDataDisplay_func func = headerDataDisplay_funcList[column()];

    return func(role());
}

QVariant AppListModelHeaderData::headerDataDecoration() const
{
    switch (column()) {
    case 1:
        return IconCache::icon(":/icons/ip_class.png");
    case 2:
        return IconCache::icon(":/icons/script.png");
    case 3:
        return IconCache::icon(":/icons/time.png");
    }
    return {};
}
