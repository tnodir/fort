#include "applistmodel.h"

#include <QFont>
#include <QIcon>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <conf/appgroup.h>
#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netutil.h>

namespace {

const auto alertColor = QColor("orange");
const auto allowColor = QColor("green");
const auto blockColor = QColor("red");
const auto killProcessColor = QColor("magenta");
const auto inactiveColor = QColor("slategray");

QString appStateIconPath(const AppRow &appRow)
{
    if (appRow.alerted)
        return ":/icons/error.png";

    if (appRow.killProcess)
        return ":/icons/scull.png";

    if (appRow.blocked)
        return ":/icons/deny.png";

    if (appRow.lanOnly)
        return ":/icons/hostname.png";

    return ":/icons/accept.png";
}

QString appScheduleIconPath(const AppRow &appRow)
{
    switch (appRow.scheduleAction) {
    case App::ScheduleBlock:
        return ":/icons/deny.png";
    case App::ScheduleAllow:
        return ":/icons/accept.png";
    case App::ScheduleRemove:
        return ":/icons/delete.png";
    case App::ScheduleKillProcess:
        return ":/icons/scull.png";
    }

    return {};
}

QColor appStateColor(const AppRow &appRow)
{
    if (appRow.killProcess)
        return killProcessColor;

    if (appRow.blocked)
        return blockColor;

    return allowColor;
}

QVariant appGroupColor(const AppRow &appRow)
{
    if (!appRow.useGroupPerm)
        return inactiveColor;

    const FirewallConf *conf = IoC<ConfAppManager>()->conf();

    const AppGroup *appGroup = conf->appGroupAt(appRow.groupIndex);
    if (!appGroup->enabled())
        return blockColor;

    return {};
}

QIcon appStateIcon(const AppRow &appRow)
{
    return IconCache::icon(appStateIconPath(appRow));
}

QIcon appParkedIcon(const AppRow &appRow)
{
    return appRow.parked ? IconCache::icon(":/icons/parking.png") : QIcon();
}

QIcon appRuleIcon(const AppRow &appRow)
{
    return (appRow.ruleId != 0) ? IconCache::icon(":/icons/script.png") : QIcon();
}

QIcon appScheduledIcon(const AppRow &appRow)
{
    if (appRow.scheduleTime.isNull())
        return QIcon();

    return IconCache::icon(appScheduleIconPath(appRow));
}

QVariant headerDataDisplayName(int /*role*/)
{
    return AppListModel::tr("Name");
}

QVariant headerDataDisplayParked(int role)
{
    return (role == Qt::ToolTipRole) ? AppListModel::tr("Parked") : QString();
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

static headerDataDisplay_func headerDataDisplay_funcList[] = {
    &headerDataDisplayName,
    &headerDataDisplayParked,
    &headerDataDisplayRule,
    &headerDataDisplayScheduled,
    &headerDataDisplayAction,
    &headerDataDisplayGroup,
    &headerDataDisplayFilePath,
    &headerDataDisplayCreationTime,
};

inline QVariant headerDataDisplay(int column, int role)
{
    const headerDataDisplay_func func = headerDataDisplay_funcList[column];

    return func(role);
}

inline QVariant headerDataDecoration(int column)
{
    switch (column) {
    case 1:
        return IconCache::icon(":/icons/parking.png");
    case 2:
        return IconCache::icon(":/icons/script.png");
    case 3:
        return IconCache::icon(":/icons/time.png");
    }
    return QVariant();
}

QVariant dataDisplayName(const AppRow &appRow, int role)
{
    return appRow.appName
            + (role == Qt::ToolTipRole && !appRow.notes.isEmpty() ? "\n\n" + appRow.notes
                                                                  : QString());
}

QVariant dataDisplayAction(const AppRow &appRow, int /*role*/)
{
    if (appRow.killProcess)
        return AppListModel::tr("Kill Process");

    if (appRow.blocked)
        return AppListModel::tr("Block");

    return AppListModel::tr("Allow");
}

QVariant dataDisplayParked(const AppRow & /*appRow*/, int /*role*/)
{
    return {};
}

QVariant dataDisplayRule(const AppRow &appRow, int role)
{
    if (role != Qt::ToolTipRole)
        return QString();

    return appRow.ruleName;
}

QVariant dataDisplayScheduled(const AppRow &appRow, int role)
{
    if (role != Qt::ToolTipRole || appRow.scheduleTime.isNull())
        return QString();

    return DateUtil::localeDateTime(appRow.scheduleTime);
}

QVariant dataDisplayGroup(const AppRow &appRow, int /*role*/)
{
    const FirewallConf *conf = IoC<ConfAppManager>()->conf();

    const AppGroup *appGroup = conf->appGroupAt(appRow.groupIndex);

    return appGroup->name();
}

QVariant dataDisplayFilePath(const AppRow &appRow, int /*role*/)
{
    return appRow.appOriginPath;
}

QVariant dataDisplayCreationTime(const AppRow &appRow, int /*role*/)
{
    return appRow.creatTime;
}

using dataDisplay_func = QVariant (*)(const AppRow &appRow, int role);

static dataDisplay_func dataDisplay_funcList[] = {
    &dataDisplayName,
    &dataDisplayParked,
    &dataDisplayRule,
    &dataDisplayScheduled,
    &dataDisplayAction,
    &dataDisplayGroup,
    &dataDisplayFilePath,
    &dataDisplayCreationTime,
};

inline QVariant dataDisplayRow(const AppRow &appRow, int column, int role)
{
    const dataDisplay_func func = dataDisplay_funcList[column];
    Q_ASSERT(func);

    return func(appRow, role);
}

}

AppListModel::AppListModel(QObject *parent) : FtsTableSqlModel(parent) { }

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
        case Qt::ToolTipRole:
            return headerDataDisplay(section, role);

        // Icon
        case Qt::DecorationRole:
            return headerDataDecoration(section);
        }
    }
    return QVariant();
}

QVariant AppListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

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

    return QVariant();
}

QVariant AppListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &appRow = appRowAt(row);
    if (appRow.isNull())
        return QVariant();

    return dataDisplayRow(appRow, column, role);
}

QVariant AppListModel::dataDecoration(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &appRow = appRowAt(row);
    if (appRow.isNull())
        return QVariant();

    switch (column) {
    case 0:
        return appIcon(appRow);
    case 1:
        return appParkedIcon(appRow);
    case 2:
        return appRuleIcon(appRow);
    case 3:
        return appScheduledIcon(appRow);
    case 4:
        return appStateIcon(appRow);
    }

    return QVariant();
}

QVariant AppListModel::dataForeground(const QModelIndex &index) const
{
    const int column = index.column();

    const int row = index.row();
    const auto &appRow = appRowAt(row);

    switch (column) {
    case 4:
        return appStateColor(appRow);
    case 5:
        return appGroupColor(appRow);
    }

    return QVariant();
}

QVariant AppListModel::dataTextAlignment(const QModelIndex &index) const
{
    const int column = index.column();

    if (column == 5) {
        return int(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    return QVariant();
}

QIcon AppListModel::appIcon(const AppRow &appRow) const
{
    if (appRow.isWildcard) {
        return IconCache::icon(":/icons/coding.png");
    }

    return appInfoCache()->appIcon(appRow.appPath);
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
    appRow.applyChild = stmt.columnBool(7);
    appRow.killChild = stmt.columnBool(8);
    appRow.lanOnly = stmt.columnBool(9);
    appRow.parked = stmt.columnBool(10);
    appRow.logBlocked = stmt.columnBool(11);
    appRow.logConn = stmt.columnBool(12);
    appRow.blocked = stmt.columnBool(13);
    appRow.killProcess = stmt.columnBool(14);
    appRow.acceptZones = stmt.columnUInt(15);
    appRow.rejectZones = stmt.columnUInt(16);
    appRow.ruleId = stmt.columnUInt(17);
    appRow.scheduleAction = stmt.columnInt(18);
    appRow.scheduleTime = stmt.columnDateTime(19);
    appRow.creatTime = stmt.columnDateTime(20);
    appRow.groupIndex = stmt.columnInt(21);
    appRow.alerted = stmt.columnBool(22);
    appRow.ruleName = stmt.columnText(23);

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
           "    t.apply_child,"
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

QString AppListModel::sqlWhereFts() const
{
    return " WHERE t.app_id IN ( SELECT rowid FROM app_fts(:match) )";
}

QString AppListModel::sqlOrderColumn() const
{
    static const QString nameColumn = "t.name";
    static const QString pathColumn = "t.path";

    static const QStringList orderColumns = {
        nameColumn, // Name
        "t.parked", // Parked
        "t.rule_id", // Rule
        "t.end_time", // Scheduled
        "alerted DESC, t.kill_process, t.blocked", // Action
        "group_index", // Group
        pathColumn, // File Path
        "t.app_id", // Creation Time ~ App ID
    };
    static const QStringList postOrderColumns = {
        pathColumn, // Name
        nameColumn, // Parked
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
