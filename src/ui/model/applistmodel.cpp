#include "applistmodel.h"

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <util/conf/confutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netutil.h>

#include "applistmodeldata.h"
#include "applistmodelheaderdata.h"

AppListModel::AppListModel(QObject *parent) : FtsTableSqlModel(parent) { }

void AppListModel::setFilters(FilterFlags v)
{
    if (m_filters == v)
        return;

    m_filters = v;
    emit filtersChanged();

    resetLater();
}

void AppListModel::setFilter(FilterFlag v, bool on)
{
    auto filters = m_filters;
    filters.setFlag(v, on);

    setFilters(filters);
}

void AppListModel::setFilterValue(FilterFlag v, Qt::CheckState checkState)
{
    m_filterValues.setFlag(v, (checkState == Qt::Checked));

    setFilter(v, (checkState != Qt::PartiallyChecked));

    resetLater();
}

void AppListModel::clearFilters()
{
    m_filterValues = FilterNone;

    setFilters(FilterNone);
}

ConfManager *AppListModel::confManager() const
{
    return IoC<ConfManager>();
}

ConfAppManager *AppListModel::confAppManager() const
{
    return IoC<ConfAppManager>();
}

AppInfoCache *AppListModel::appInfoCache() const
{
    return IoC<AppInfoCache>();
}

SqliteDb *AppListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void AppListModel::initialize()
{
    setSortColumn(7);
    setSortOrder(Qt::DescendingOrder);

    connect(confManager(), &ConfManager::confChanged, this, &AppListModel::refresh);

    connect(confAppManager(), &ConfAppManager::appsChanged, this, &TableItemModel::reset);
    connect(confAppManager(), &ConfAppManager::appUpdated, this, &TableItemModel::refresh);

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppListModel::refresh);
}

int AppListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 8;
}

QVariant AppListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        // Label
        case Qt::DisplayRole:
        case Qt::ToolTipRole: {
            const AppListModelHeaderData data(section, role);

            return data.headerDataDisplay();
        }

        // Icon
        case Qt::DecorationRole: {
            const AppListModelHeaderData data(section, role);

            return data.headerDataDecoration();
        }
        }
    }
    return {};
}

QVariant AppListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index, role);

    // Icon
    case Qt::DecorationRole:
        return dataDecoration(index);

    // Foreground
    case Qt::ForegroundRole:
        return dataForeground(index);

    // Text Alignment
    case Qt::TextAlignmentRole:
        return dataTextAlignment(index);
    }

    return {};
}

QVariant AppListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();

    const auto &appRow = appRowAt(row);
    if (appRow.isNull())
        return {};

    const AppListModelData data(appRow, index, role);

    return data.dataDisplayRow();
}

QVariant AppListModel::dataDecoration(const QModelIndex &index) const
{
    const int row = index.row();

    const auto &appRow = appRowAt(row);
    if (appRow.isNull())
        return {};

    const AppListModelData data(appRow, index);

    return data.dataDecorationIcon();
}

QVariant AppListModel::dataForeground(const QModelIndex &index) const
{
    const int row = index.row();

    const auto &appRow = appRowAt(row);
    if (appRow.isNull())
        return {};

    const AppListModelData data(appRow, index);

    return data.dataForeground();
}

QVariant AppListModel::dataTextAlignment(const QModelIndex &index) const
{
    const int column = index.column();

    if (column == 5) {
        return int(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    return {};
}

bool AppListModel::updateAppRow(const QString &sql, const QVariantHash &vars, AppRow &appRow) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepareRow(stmt)) {
        appRow.invalidate();
        return false;
    }

    appRow.appId = stmt.columnInt64(0);
    appRow.appOriginPath = stmt.columnText(1);
    appRow.appPath = stmt.columnText(2);
    appRow.appName = stmt.columnText(3);
    appRow.notes = stmt.columnText(4);
    appRow.isWildcard = stmt.columnBool(5);
    appRow.useGroupPerm = stmt.columnBool(6);
    appRow.applyParent = stmt.columnBool(7);
    appRow.applyChild = stmt.columnBool(8);
    appRow.applySpecChild = stmt.columnBool(9);
    appRow.killChild = stmt.columnBool(10);
    appRow.lanOnly = stmt.columnBool(11);
    appRow.parked = stmt.columnBool(12);
    appRow.logBlocked = stmt.columnBool(13);
    appRow.logConn = stmt.columnBool(14);
    appRow.blocked = stmt.columnBool(15);
    appRow.killProcess = stmt.columnBool(16);
    appRow.acceptZones = stmt.columnUInt(17);
    appRow.rejectZones = stmt.columnUInt(18);
    appRow.ruleId = stmt.columnUInt(19);
    appRow.scheduleAction = stmt.columnInt(20);
    appRow.scheduleTime = stmt.columnDateTime(21);
    appRow.creatTime = stmt.columnDateTime(22);
    appRow.groupIndex = stmt.columnInt(23);
    appRow.alerted = stmt.columnBool(24);
    appRow.ruleName = stmt.columnText(25);

    return true;
}

const AppRow &AppListModel::appRowAt(int row) const
{
    updateRowCache(row);

    return m_appRow;
}

AppRow AppListModel::appRowById(qint64 appId) const
{
    QVariantHash vars;
    vars.insert(":app_id", appId);

    AppRow appRow;
    updateAppRow(sqlBase() + " WHERE t.app_id = :app_id;", vars, appRow);
    return appRow;
}

AppRow AppListModel::appRowByPath(const QString &appPath) const
{
    QString normPath;
    const qint64 appId = confAppManager()->appIdByPath(appPath, normPath);

    AppRow appRow = appRowById(appId);
    if (appRow.appId == 0) {
        appRow.isWildcard = ConfUtil::matchWildcard(normPath).hasMatch();
        appRow.appOriginPath = appPath;
        appRow.appPath = normPath;
    }
    return appRow;
}

bool AppListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    return updateAppRow(sql(), vars, m_appRow);
}

QString AppListModel::sqlBase() const
{
    return "SELECT"
           "    t.app_id,"
           "    t.origin_path,"
           "    t.path,"
           "    t.name,"
           "    t.notes,"
           "    t.is_wildcard,"
           "    t.use_group_perm,"
           "    t.apply_parent,"
           "    t.apply_child,"
           "    t.apply_spec_child,"
           "    t.kill_child,"
           "    t.lan_only,"
           "    t.parked,"
           "    t.log_blocked,"
           "    t.log_conn,"
           "    t.blocked,"
           "    t.kill_process,"
           "    t.accept_zones,"
           "    t.reject_zones,"
           "    t.rule_id,"
           "    t.end_action,"
           "    t.end_time,"
           "    t.creat_time,"
           "    g.order_index as group_index,"
           "    (a.app_id IS NOT NULL) as alerted,"
           "    r.name as rule_name"
           "  FROM app t"
           "    JOIN app_group g ON g.app_group_id = t.app_group_id"
           "    LEFT JOIN app_alert a ON a.app_id = t.app_id"
           "    LEFT JOIN rule r ON r.rule_id = t.rule_id";
}

QString AppListModel::sqlWhere() const
{
    QString sql = FtsTableSqlModel::sqlWhere();

    if (filters() != FilterNone) {
        QStringList list;
        addSqlFilter(list, "alerted", FilterAlerted);
        addSqlFilter(list, "t.is_wildcard", FilterWildcard);
        addSqlFilter(list, "t.parked", FilterParked);

        sql += QLatin1String(sql.isEmpty() ? " WHERE " : " AND ") + list.join(" AND ");
    }

    return sql;
}

QString AppListModel::sqlWhereFts() const
{
    return " WHERE t.app_id IN ( SELECT rowid FROM app_fts(:match) )";
}

QString AppListModel::sqlOrderColumn() const
{
    static const QString nameColumn = "EXT_LOWER(t.name)";
    static const QString pathColumn = "t.path";

    static const QStringList orderColumns = {
        nameColumn, // Name
        "t.accept_zones, t.reject_zones", // Zones
        "t.rule_id", // Rule
        "t.end_time", // Scheduled
        "alerted DESC, t.kill_process, t.blocked", // Action
        "group_index", // Group
        pathColumn, // File Path
        "t.app_id", // Creation Time ~ App ID
    };
    static const QStringList postOrderColumns = {
        pathColumn, // Name
        nameColumn, // Zones
        nameColumn, // Rule
        nameColumn, // Scheduled
        nameColumn, // Action
        nameColumn, // Group
        nameColumn, // File Path
        nameColumn, // Creation Time ~ App ID
    };

    Q_ASSERT(sortColumn() >= 0 && sortColumn() < orderColumns.size()
            && orderColumns.size() == postOrderColumns.size());

    const auto &columnsStr = orderColumns.at(sortColumn());
    const auto &postColumnsStr = postOrderColumns.at(sortColumn());

    return columnsStr + sqlOrderAsc() + ", " + postColumnsStr;
}

void AppListModel::addSqlFilter(QStringList &list, const QString &name, FilterFlag flag) const
{
    if (filters().testFlag(flag)) {
        const QLatin1String value(filterValues().testFlag(flag) ? "1" : "0");

        list << QString("%1 = %2").arg(name, value);
    }
}
