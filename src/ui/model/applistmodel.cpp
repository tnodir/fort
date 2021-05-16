#include "applistmodel.h"

#include <QFont>
#include <QIcon>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../appinfo/appinfocache.h"
#include "../appinfo/appinfoutil.h"
#include "../conf/appgroup.h"
#include "../conf/confmanager.h"
#include "../conf/firewallconf.h"
#include "../log/logentryblocked.h"
#include "../util/fileutil.h"
#include "../util/guiutil.h"
#include "../util/iconcache.h"
#include "../util/net/netutil.h"

namespace {

const auto alertColor = QColor("orange");
const auto allowColor = QColor("green");
const auto blockColor = QColor("red");
const auto inactiveColor = QColor("slategray");

}

AppListModel::AppListModel(ConfManager *confManager, QObject *parent) :
    TableSqlModel(parent), m_confManager(confManager)
{
}

FirewallConf *AppListModel::conf() const
{
    return confManager()->conf();
}

SqliteDb *AppListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void AppListModel::setAppInfoCache(AppInfoCache *v)
{
    m_appInfoCache = v;

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppListModel::refresh);
}

void AppListModel::initialize()
{
    setSortColumn(4);
    setSortOrder(Qt::DescendingOrder);

    connect(confManager(), &ConfManager::confChanged, this, [&](bool onlyFlags = false) {
        if (!onlyFlags) {
            refresh();
        }
    });
    connect(confManager(), &ConfManager::appEndTimesUpdated, this, &AppListModel::refresh);
    connect(confManager(), &ConfManager::appAdded, this, &TableSqlModel::reset);
    connect(confManager(), &ConfManager::appUpdated, this, &TableSqlModel::refresh);
}

void AppListModel::handleLogBlocked(const LogEntryBlocked &logEntry)
{
    const QString appPath = logEntry.path();

    if (confManager()->appIdByPath(appPath) > 0)
        return; // already added by user

    constexpr int groupIndex = 0;
    const auto groupId = appGroupAt(groupIndex)->id();
    const auto appName = appInfoCache()->appName(appPath);

    confManager()->addApp(
            appPath, appName, QDateTime(), groupId, groupIndex, false, logEntry.blocked(), true);
}

int AppListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant AppListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        return headerDataDisplay(section, role);
    }
    return QVariant();
}

QVariant AppListModel::headerDataDisplay(int section, int role) const
{
    switch (section) {
    case 0:
        return tr("Program");
    case 1:
        return tr("Group");
    case 2:
        return tr("State");
    case 3:
        return (role == Qt::DisplayRole) ? tr("Bl.") : tr("Block scheduled");
    case 4:
        return tr("Creation Time");
    default:
        Q_UNREACHABLE();
        return QVariant();
    }
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

    // Font
    case Qt::FontRole:
        return dataFont(index);

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

    const auto appRow = appRowAt(row);

    switch (column) {
    case 0:
        return appRow.appName;
    case 1:
        return appGroupAt(appRow.groupIndex)->name();
    case 2:
        return appStateText(appRow);
    case 3:
        return dataDisplayEndTime(appRow, role);
    case 4:
        return appRow.creatTime;
    }

    return QVariant();
}

QVariant AppListModel::dataDisplayEndTime(const AppRow &appRow, int role) const
{
    if (role == Qt::ToolTipRole && !appRow.endTime.isNull()) {
        return appRow.endTime;
    }
    return QVariant();
}

QVariant AppListModel::dataDecoration(const QModelIndex &index) const
{
    const int column = index.column();

    if (column == 0 || column == 2 || column == 3) {
        const int row = index.row();
        const auto appRow = appRowAt(row);

        switch (column) {
        case 0:
            return appInfoCache()->appIcon(appRow.appPath);
        case 2:
            return appStateIcon(appRow);
        case 3:
            return appEndTimeIcon(appRow);
        }
    }

    return QVariant();
}

QVariant AppListModel::dataFont(const QModelIndex &index) const
{
    const int column = index.column();

    switch (column) {
    case 2: {
        QFont font;
        font.setWeight(QFont::DemiBold);
        return font;
    }
    case 4: {
        QFont font;
        font.setPointSize(8);
        return font;
    }
    }

    return QVariant();
}

QVariant AppListModel::dataForeground(const QModelIndex &index) const
{
    const int column = index.column();

    if (column == 1 || column == 2) {
        const int row = index.row();
        const auto appRow = appRowAt(row);

        switch (column) {
        case 1:
            return appGroupColor(appRow);
        case 2:
            return appStateColor(appRow);
        }
    }

    return QVariant();
}

QVariant AppListModel::dataTextAlignment(const QModelIndex &index) const
{
    const int column = index.column();

    if (column == 1) {
        return int(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    return QVariant();
}

QString AppListModel::appStateText(const AppRow &appRow)
{
    if (appRow.alerted)
        return tr("Alert");
    if (appRow.blocked)
        return tr("Block");
    return tr("Allow");
}

QColor AppListModel::appGroupColor(const AppRow &appRow) const
{
    if (!appRow.useGroupPerm)
        return inactiveColor;
    if (!appGroupAt(appRow.groupIndex)->enabled())
        return blockColor;
    return {};
}

QColor AppListModel::appStateColor(const AppRow &appRow)
{
    if (appRow.alerted)
        return alertColor;
    if (appRow.blocked)
        return blockColor;
    return allowColor;
}

QIcon AppListModel::appStateIcon(const AppRow &appRow)
{
    return IconCache::icon(appRow.blocked ? ":/icons/sign-ban.png" : ":/icons/sign-check.png");
}

QIcon AppListModel::appEndTimeIcon(const AppRow &appRow)
{
    return appRow.endTime.isNull() ? QIcon() : IconCache::icon(":/icons/clock.png");
}

bool AppListModel::updateAppRow(const QString &sql, const QVariantList &vars, AppRow &appRow) const
{
    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql, vars) && stmt.step() == SqliteStmt::StepRow))
        return false;

    appRow.appId = stmt.columnInt64(0);
    appRow.groupIndex = stmt.columnInt(1);
    appRow.appPath = stmt.columnText(2);
    appRow.appName = stmt.columnText(3);
    appRow.useGroupPerm = stmt.columnBool(4);
    appRow.blocked = stmt.columnBool(5);
    appRow.alerted = stmt.columnBool(6);
    appRow.endTime = stmt.columnDateTime(7);
    appRow.creatTime = stmt.columnDateTime(8);

    return true;
}

const AppRow &AppListModel::appRowAt(int row) const
{
    updateRowCache(row);

    return m_appRow;
}

AppRow AppListModel::appRowById(qint64 appId) const
{
    AppRow appRow;
    updateAppRow(sqlBase() + " WHERE t.app_id = ?1;", { appId }, appRow);
    return appRow;
}

AppRow AppListModel::appRowByPath(const QString &appPath) const
{
    AppRow appRow;
    if (!updateAppRow(sqlBase() + " WHERE t.path = ?1;", { appPath }, appRow)) {
        appRow.appPath = appPath;
    }
    return appRow;
}

bool AppListModel::addApp(const QString &appPath, const QString &appName, const QDateTime &endTime,
        int groupIndex, bool useGroupPerm, bool blocked)
{
    const auto groupId = appGroupAt(groupIndex)->id();

    return confManager()->addApp(
            appPath, appName, endTime, groupId, groupIndex, useGroupPerm, blocked);
}

bool AppListModel::updateApp(qint64 appId, const QString &appPath, const QString &appName,
        const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool blocked)
{
    const auto groupId = appGroupAt(groupIndex)->id();

    return confManager()->updateApp(
            appId, appPath, appName, endTime, groupId, groupIndex, useGroupPerm, blocked);
}

bool AppListModel::updateAppName(qint64 appId, const QString &appName)
{
    return confManager()->updateAppName(appId, appName);
}

void AppListModel::deleteApp(qint64 appId, const QString &appPath, int row)
{
    doBeginRemoveRows(row, row);

    confManager()->deleteApp(appId, appPath);

    doEndRemoveRows();
}

void AppListModel::purgeApps()
{
    for (int row = rowCount(); --row >= 0;) {
        const auto appRow = appRowAt(row);
        const auto appPath = appRow.appPath;
        if (!FileUtil::fileExists(appPath)) {
            AppInfo appInfo;
            if (!AppInfoUtil::getInfo(appPath, appInfo)) {
                deleteApp(appRow.appId, appPath, row);
            }
        }
    }
}

bool AppListModel::updateTableRow(int row) const
{
    return updateAppRow(sql(), { row }, m_appRow);
}

QString AppListModel::sqlBase() const
{
    return "SELECT"
           "    t.app_id,"
           "    g.order_index as group_index,"
           "    t.path,"
           "    t.name,"
           "    t.use_group_perm,"
           "    t.blocked,"
           "    (alert.app_id IS NOT NULL) as alerted,"
           "    t.end_time,"
           "    t.creat_time"
           "  FROM app t"
           "    JOIN app_group g ON g.app_group_id = t.app_group_id"
           "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id";
}

QString AppListModel::sqlOrderColumn() const
{
    QString columnsStr;
    switch (sortColumn()) {
    case 0: // Program
        columnsStr = "4 " + sqlOrderAsc() + ", 3";
        break;
    case 1: // Group
        columnsStr = "2";
        break;
    case 2: // State
        columnsStr = "6 " + sqlOrderAsc() + ", 7";
        break;
    case 3: // End Time
        columnsStr = "8";
        break;
    default: // Creation Time
        columnsStr = "1"; // App ID
        break;
    }

    return columnsStr;
}

const AppGroup *AppListModel::appGroupAt(int index) const
{
    const auto appGroups = conf()->appGroups();
    if (index < 0 || index >= appGroups.size()) {
        static const AppGroup g_nullAppGroup;
        return &g_nullAppGroup;
    }
    return appGroups.at(index);
}

QStringList AppListModel::appGroupNames() const
{
    QStringList list;
    for (const auto &appGroup : conf()->appGroups()) {
        list.append(appGroup->name());
    }
    return list;
}
