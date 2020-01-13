#include "applistmodel.h"

#include <QIcon>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../../conf/appgroup.h"
#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../util/app/appinfocache.h"
#include "../../util/fileutil.h"
#include "../../util/guiutil.h"
#include "../../util/net/netutil.h"
#include "../logentryblocked.h"

#define IP_LIST_SIZE_MAX    64

AppListModel::AppListModel(ConfManager *confManager,
                           QObject *parent) :
    TableItemModel(parent),
    m_confManager(confManager)
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
    connect(confManager(), &ConfManager::confSaved, this, &AppListModel::reset);
    connect(confManager(), &ConfManager::appEndTimesUpdated, this, &AppListModel::refresh);
}

void AppListModel::addLogEntry(const LogEntryBlocked &logEntry)
{
    const QString appPath = logEntry.path();

#if 0
    const QString ipText = NetUtil::ip4ToText(logEntry.ip())
            + ", " + NetUtil::protocolName(logEntry.proto())
            + ':' + QString::number(logEntry.port());
#endif

    const auto groupId = appGroupAt(0)->id();

    if (confManager()->addApp(appPath, QDateTime(),
                              groupId, true, true, true)) {
        reset();
    }
}

int AppListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    if (m_appCount < 0) {
        m_appCount = sqliteDb()->executeEx(sqlCount().toLatin1()).toInt();
    }

    return m_appCount;
}

int AppListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant AppListModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole: {
            switch (section) {
            case 0: return tr("Program");
            case 1: return tr("Group");
            case 2: return tr("State");
            case 3: return tr("End Time");
            case 4: return tr("Creation Time");
            }
            break;
        }
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
    case Qt::ToolTipRole: {
        const int row = index.row();
        const int column = index.column();

        const auto appRow = appRowAt(row);

        switch (column) {
        case 0: {
            const auto appInfo = appInfoCache()->appInfo(appRow.appPath);
            if (!appInfo.fileDescription.isEmpty()) {
                return appInfo.fileDescription;
            }

            return FileUtil::fileName(appRow.appPath);
        }
        case 1: return appGroupAt(appRow.groupIndex)->name();
        case 2: return appStateToString(appRow.state);
        case 3: return appRow.endTime.isValid()
                    ? appRow.endTime : QVariant();
        case 4: return appRow.creatTime;
        }

        break;
    }

    // Icon
    case Qt::DecorationRole: {
        const int row = index.row();
        const int column = index.column();

        const auto appRow = appRowAt(row);

        switch (column) {
        case 0: {
            const auto appPath = appRow.appPath;
            const auto appInfo = appInfoCache()->appInfo(appPath);
            const auto appIcon = appInfoCache()->appIcon(appInfo);
            if (!appIcon.isNull()) {
                return QIcon(QPixmap::fromImage(appIcon));
            }

            return QIcon(":/images/application-window-96.png");
        }
        case 2: {
            QString iconPath;
            switch (appRow.state) {
            case AppAlert: return QIcon(":/images/error.png");
            case AppBlock: return QIcon(":/images/stop.png");
            case AppAllow: return QIcon(":/images/arrow_switch.png");
            }
        }
        }

        break;
    }

    // Text Alignment
    case Qt::TextAlignmentRole: {
        const int column = index.column();

        if (column >= 1 && column <= 2) {
            return int(Qt::AlignHCenter | Qt::AlignVCenter);
        }

        break;
    }
    }

    return QVariant();
}

Qt::ItemFlags AppListModel::flags(const QModelIndex &index) const
{
    auto flags = TableItemModel::flags(index);

    if ((flags & Qt::ItemIsEnabled)
            && index.column() == 2) {
        const auto appRow = appRowAt(index.row());
        if (appRow.useGroupPerm
                && !appGroupAt(appRow.groupIndex)->enabled()) {
            flags ^= Qt::ItemIsEnabled;
        }
    }

    return flags;
}

void AppListModel::sort(int column, Qt::SortOrder order)
{
    if (m_sortColumn != column || m_sortOrder != order) {
        m_sortColumn = column;
        m_sortOrder = order;

        reset();
    }
}

const AppRow &AppListModel::appRowAt(int row) const
{
    updateRowCache(row);

    return m_appRow;
}

bool AppListModel::addApp(const QString &appPath, int groupIndex,
                          bool useGroupPerm, bool blocked,
                          const QDateTime &endTime)
{
    const auto groupId = appGroupAt(groupIndex)->id();

    if (confManager()->addApp(appPath, endTime, groupId,
                              useGroupPerm, blocked, false)) {
        reset();

        return confManager()->updateDriverUpdateApp(
                    appPath, groupIndex, useGroupPerm, blocked, false);
    }
    return false;
}

bool AppListModel::updateApp(qint64 appId, const QString &appPath,
                             int groupIndex, bool useGroupPerm, bool blocked,
                             const QDateTime &endTime)
{
    const auto groupId = appGroupAt(groupIndex)->id();

    if (confManager()->updateApp(appId, endTime, groupId,
                                 useGroupPerm, blocked)) {
        refresh();

        return confManager()->updateDriverUpdateApp(
                    appPath, groupIndex, useGroupPerm, blocked, false);
    }
    return false;
}

void AppListModel::deleteApp(qint64 appId, const QString &appPath, int row)
{
    beginRemoveRows(QModelIndex(), row, row);

    if (confManager()->deleteApp(appId)) {
        confManager()->updateDriverDeleteApp(appPath);

        invalidateRowCache();
        removeRow(row);
    }

    endRemoveRows();
}

void AppListModel::reset()
{
    invalidateRowCache();
    TableItemModel::reset();
}

void AppListModel::refresh()
{
    invalidateRowCache();
    TableItemModel::refresh();
}

void AppListModel::invalidateRowCache()
{
    m_appCount = -1;
    m_appRow.invalidate();
}

void AppListModel::updateRowCache(int row) const
{
    if (m_appRow.isValid(row))
        return;

    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql().toLatin1(), {row})
          && stmt.step() == SqliteStmt::StepRow))
        return;

    m_appRow.appId = stmt.columnInt64(0);
    m_appRow.groupIndex = stmt.columnInt(1);
    m_appRow.appPath = stmt.columnText(2);
    m_appRow.useGroupPerm = stmt.columnBool(3);
    const bool blocked = stmt.columnBool(4);
    const bool alerted = stmt.columnBool(5);
    m_appRow.endTime = stmt.columnDateTime(6);
    m_appRow.creatTime = stmt.columnDateTime(7);

    m_appRow.state = alerted ? AppAlert : (blocked ? AppBlock : AppAllow);
    m_appRow.row = row;
}

QString AppListModel::sqlCount() const
{
    return "SELECT count(*) FROM (" + sqlBase() + ");";
}

QString AppListModel::sql() const
{
    return sqlBase() + sqlOrder() + " LIMIT 1 OFFSET ?1;"
            ;
}

QString AppListModel::sqlBase() const
{
    return
            "SELECT"
            "    t.app_id,"
            "    g.order_index as group_index,"
            "    t.path,"
            "    t.use_group_perm,"
            "    t.blocked,"
            "    (alert.app_id IS NOT NULL) as alerted,"
            "    t.end_time,"
            "    t.creat_time"
            "  FROM app t"
            "    JOIN app_group g ON g.app_group_id = t.app_group_id"
            "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id"
            ;
}

QString AppListModel::sqlOrder() const
{
    QString columnsStr = "1";
    switch (m_sortColumn) {
    case 0: columnsStr = "3"; break;  // Program
    case 1: columnsStr = "2"; break;  // Group
    case 2: columnsStr = "5, 6"; break;  // State
    case 3: columnsStr = "7"; break;  // End Time
    case 4: columnsStr = "1"; break;  // Creation Time
    }

    const QString orderStr = (m_sortOrder == Qt::AscendingOrder)
            ? "ASC" : "DESC";

    return QString(" ORDER BY %1 %2").arg(columnsStr, orderStr);
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

QString AppListModel::appStateToString(AppState state) const
{
    switch (state) {
    case AppAlert: return tr("Alert");
    case AppBlock: return tr("Block");
    case AppAllow: return tr("Allow");
    }

    Q_UNREACHABLE();
    return QString();
}
