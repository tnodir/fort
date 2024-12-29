#include "applistmodelheaderdata.h"

#include <QFont>

#include <util/iconcache.h>

#include "applistcolumn.h"
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
    switch (AppListColumn(column())) {
    case AppListColumn::Zones:
        return IconCache::icon(":/icons/ip_class.png");
    case AppListColumn::Rule:
        return IconCache::icon(":/icons/script.png");
    case AppListColumn::Scheduled:
        return IconCache::icon(":/icons/time.png");
    }
    return {};
}
