#include "applistmodel.h"

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <manager/translationmanager.h>
#include <util/conf/confutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netutil.h>

#include "applistmodeldata.h"
#include "applistmodelheaderdata.h"

AppListModel::AppListModel(QObject *parent) : FtsTableSqlModel(parent) { }

void AppListModel::setSortState(SortState v)
{
    if (m_sortState == v)
        return;

    m_sortState = v;
    emit sortStateChanged();

    resetLater();
}

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
    setSortColumn(int(AppListColumn::CreationTime));
    setSortOrder(Qt::DescendingOrder);

    connect(confManager(), &ConfManager::confChanged, this, &AppListModel::refresh);

    connect(confAppManager(), &ConfAppManager::appsChanged, this, &TableItemModel::reset);
    connect(confAppManager(), &ConfAppManager::appUpdated, this, &TableItemModel::refresh);

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppListModel::refresh);
}

int AppListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(AppListColumn::Count);
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
    const AppListModelData data = appDataAt(index, role);

    return data.dataDisplayRow();
}

QVariant AppListModel::dataDecoration(const QModelIndex &index) const
{
    const AppListModelData data = appDataAt(index);

    return data.dataDecorationIcon();
}

QVariant AppListModel::dataForeground(const QModelIndex &index) const
{
    const AppListModelData data = appDataAt(index);

    return data.dataForeground();
}

QVariant AppListModel::dataTextAlignment(const QModelIndex &index) const
{
    const AppListModelData data = appDataAt(index);

    return data.dataTextAlignment();
}

AppListModelData AppListModel::appDataAt(const QModelIndex &index, int role) const
{
    const int row = index.row();

    const auto &appRow = appRowAt(row);

    return AppListModelData(appRow, index, role);
}

const AppRow &AppListModel::appRowAt(int row) const
{
    updateRowCache(row);

    return m_appRow;
}

AppStatesCount AppListModel::appStatesCount() const
{
    const auto sql = "SELECT"
                     "    SUM(CASE WHEN blocked = 0 THEN 1 ELSE 0 END)," // Allowed
                     "    SUM(CASE WHEN blocked = 1 THEN 1 ELSE 0 END)," // Blocked
                     "    SUM(CASE WHEN alerted = 1 THEN 1 ELSE 0 END)" // Alerted
                     "  FROM ("
            + sqlBase() + sqlWhere() + ");";

    QVariantHash vars;
    fillQueryVars(vars);

    const auto list = DbQuery(sqliteDb()).sql(sql).vars(vars).execute(3).toList();

    return {
        .allowed = list.value(0).toInt(),
        .blocked = list.value(1).toInt(),
        .alerted = list.value(2).toInt(),
    };
}

bool AppListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql()).vars(vars).prepareRow(stmt)) {
        m_appRow.invalidate();
        return false;
    }

    m_appRow.appId = stmt.columnInt64(0);
    m_appRow.appOriginPath = stmt.columnText(1);
    m_appRow.appPath = stmt.columnText(2);
    m_appRow.appName = stmt.columnText(3);
    m_appRow.notes = stmt.columnText(4);
    m_appRow.isWildcard = stmt.columnBool(5);
    m_appRow.applyParent = stmt.columnBool(6);
    m_appRow.applyChild = stmt.columnBool(7);
    m_appRow.applySpecChild = stmt.columnBool(8);
    m_appRow.killChild = stmt.columnBool(9);
    m_appRow.lanOnly = stmt.columnBool(10);
    m_appRow.parked = stmt.columnBool(11);
    m_appRow.logAllowedConn = stmt.columnBool(12);
    m_appRow.logBlockedConn = stmt.columnBool(13);
    m_appRow.blocked = stmt.columnBool(14);
    m_appRow.killProcess = stmt.columnBool(15);
    m_appRow.zones.accept_mask = stmt.columnUInt(16);
    m_appRow.zones.reject_mask = stmt.columnUInt(17);
    m_appRow.ruleId = stmt.columnUInt(18);
    m_appRow.scheduleAction = stmt.columnInt(19);
    m_appRow.scheduleTime = stmt.columnDateTime(20);
    m_appRow.creatTime = stmt.columnDateTime(21);
    m_appRow.groupIndex = stmt.columnInt(22);
    m_appRow.alerted = stmt.columnBool(23);

    return true;
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
           "    t.apply_parent,"
           "    t.apply_child,"
           "    t.apply_spec_child,"
           "    t.kill_child,"
           "    t.lan_only,"
           "    t.parked,"
           "    t.log_allowed_conn,"
           "    t.log_blocked_conn,"
           "    t.blocked,"
           "    t.kill_process,"
           "    t.accept_zones,"
           "    t.reject_zones,"
           "    t.rule_id,"
           "    t.end_action,"
           "    t.end_time,"
           "    t.creat_time,"
           "    g.order_index as group_index,"
           "    (a.app_id IS NOT NULL) as alerted"
           "  FROM app t"
           "    JOIN app_group g ON g.app_group_id = t.app_group_id"
           "    LEFT JOIN app_alert a ON a.app_id = t.app_id";
}

QString AppListModel::sqlWhere() const
{
    QString sql = FtsTableSqlModel::sqlWhere();

    if (filters() != FilterNone) {
        QStringList list;
        addSqlFilter(list, "alerted", FilterAlerted);
        addSqlFilter(list, "t.is_wildcard", FilterWildcard);
        addSqlFilter(list, "t.parked", FilterParked);
        addSqlFilter(list, "t.kill_process", FilterKillProcess);

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
    static const QString nameColumn = "lower(t.name)";
    static const QString pathColumn = "t.path";

    static const QStringList sortStateColumns = {
        "t.blocked ASC", // SortAllowed
        "t.blocked DESC", // SortBlocked
        "alerted DESC", // SortAlerted
    };

    static const QStringList orderColumns = {
        nameColumn, // Name
        "t.accept_zones, t.reject_zones", // Zones
        "t.rule_id", // Rule
        "t.end_action, t.end_time", // Scheduled
        "t.blocked", // Action
        "group_index", // Group
        pathColumn, // File Path
        "t.app_id", // Creation Time ~ App ID
        "t.notes", // Notes
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
        nameColumn, // Notes
    };

    Q_ASSERT(sortColumn() >= 0 && sortColumn() < orderColumns.size()
            && orderColumns.size() == postOrderColumns.size());

    const auto sortStateStr =
            (sortState() != SortNone) ? sortStateColumns.at(sortState() - 1) + ", " : QString();

    const auto &columnsStr = orderColumns.at(sortColumn());
    const auto &postColumnsStr = postOrderColumns.at(sortColumn());

    return sortStateStr + columnsStr + sqlOrderAsc() + ", " + postColumnsStr;
}

void AppListModel::addSqlFilter(QStringList &list, const QString &name, FilterFlag flag) const
{
    if (filters().testFlag(flag)) {
        const QLatin1String value(filterValues().testFlag(flag) ? "1" : "0");

        list << QString("%1 = %2").arg(name, value);
    }
}

QString AppListModel::columnName(const AppListColumn column)
{
    static QStringList g_columnNames;
    static int g_language = -1;

    const int language = IoC<TranslationManager>()->language();
    if (g_language != language) {
        g_language = language;

        g_columnNames = {
            tr("Name"),
            tr("Zones"),
            tr("Rule"),
            tr("Scheduled"),
            tr("Action"),
            tr("Group"),
            tr("File Path"),
            tr("Creation Time"),
            tr("Notes"),
        };
    }

    return g_columnNames.value(int(column));
}
