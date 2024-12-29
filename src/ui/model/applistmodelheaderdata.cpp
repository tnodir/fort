#include "applistmodelheaderdata.h"

#include <QFont>

#include <util/iconcache.h>

#include "applistmodel.h"

AppListModelHeaderData::AppListModelHeaderData(int column, int role) :
    m_column(column), m_role(role)
{
}

QVariant AppListModelHeaderData::headerDataDisplay() const
{
    if (role() != Qt::ToolTipRole) {
        if (column() >= int(AppListColumn::Zones) && column() <= int(AppListColumn::Scheduled))
            return {};
    }

    return AppListModel::columnName(AppListColumn(column()));
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
