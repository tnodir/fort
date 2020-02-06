#include "applistmodel.h"

#include <QFont>
#include <QIcon>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../conf/appgroup.h"
#include "../conf/confmanager.h"
#include "../conf/firewallconf.h"
#include "../log/logentryblocked.h"
#include "../util/app/appinfocache.h"
#include "../util/app/apputil.h"
#include "../util/fileutil.h"
#include "../util/guiutil.h"
#include "../util/net/netutil.h"

namespace {

const auto alertColor = QColor("orange");
const auto allowColor = QColor("green");
const auto blockColor = QColor("red");
const auto inactiveColor = QColor("slategray");

}

AppListModel::AppListModel(ConfManager *confManager,
                           QObject *parent) :
    TableSqlModel(parent),
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
    setSortColumn(5);
    setSortOrder(Qt::DescendingOrder);

    connect(confManager(), &ConfManager::confSaved, this, &AppListModel::refresh);
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

    if (confManager()->addApp(appPath, QString(), QDateTime(),
                              groupId, false, logEntry.blocked(), true)) {
        reset();
    }
}

int AppListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant AppListModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const
{
    if (orientation == Qt::Horizontal
            && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return tr("Program");
        case 1: return tr("Group");
        case 2: return tr("State");
        case 3: return tr("Bl.");
        case 4: return tr("Creation Time");
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
            auto appName = appRow.appName;
            if (appName.isEmpty()) {
                const auto appInfo = appInfoCache()->appInfo(appRow.appPath);
                appName = !appInfo.fileDescription.isEmpty()
                        ? appInfo.fileDescription
                        : FileUtil::fileName(appRow.appPath);

                if (appInfo.iconId != 0) {
                    confManager()->updateAppName(appRow.appId, appName);
                }
            }
            return appName;
        }
        case 1: return appGroupAt(appRow.groupIndex)->name();
        case 2: {
            if (appRow.alerted) return tr("Alert");
            if (appRow.blocked) return tr("Block");
            return tr("Allow");
        }
        case 3: return (role == Qt::DisplayRole || appRow.endTime.isNull())
                    ? QVariant() : appRow.endTime;
        case 4: return appRow.creatTime;
        }

        break;
    }

    // Icon
    case Qt::DecorationRole: {
        const int column = index.column();

        if (column == 0 || column == 2 || column == 3) {
            const int row = index.row();
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
            case 2:
                return appRow.blocked ? QIcon(":/images/stop.png")
                                      : QIcon(":/images/arrow_switch.png");
            case 3:
                return appRow.endTime.isNull() ? QVariant()
                                               : QIcon(":/images/clock_stop.png");
            }
        }

        break;
    }

    // Font
    case Qt::FontRole: {
        const int column = index.column();

        if (column == 2) {
            QFont font;
            font.setWeight(QFont::DemiBold);
            return font;
        }

        break;
    }

    // Foreground
    case Qt::ForegroundRole: {
        const int column = index.column();

        if (column == 1 || column == 2) {
            const int row = index.row();
            const auto appRow = appRowAt(row);

            switch (column) {
            case 1: {
                if (!appRow.useGroupPerm)
                    return inactiveColor;
                if (!appGroupAt(appRow.groupIndex)->enabled())
                    return blockColor;
                break;
            }
            case 2:
                if (appRow.alerted) return alertColor;
                if (appRow.blocked) return blockColor;
                return allowColor;
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

const AppRow &AppListModel::appRowAt(int row) const
{
    updateRowCache(row);

    return m_appRow;
}

bool AppListModel::addApp(const QString &appPath, const QString &appName,
                          const QDateTime &endTime, int groupIndex,
                          bool useGroupPerm, bool blocked)
{
    if (!confManager()->updateDriverUpdateApp(
                appPath, groupIndex, useGroupPerm, blocked, true))
        return false;

    const auto groupId = appGroupAt(groupIndex)->id();

    if (confManager()->addApp(appPath, appName, endTime, groupId,
                              useGroupPerm, blocked)) {
        reset();
        return true;
    }

    return false;
}

bool AppListModel::updateApp(qint64 appId, const QString &appPath, const QString &appName,
                             const QDateTime &endTime, int groupIndex,
                             bool useGroupPerm, bool blocked, bool updateDriver)
{
    if (updateDriver && !confManager()->updateDriverUpdateApp(
                appPath, groupIndex, useGroupPerm, blocked, false))
        return false;

    const auto groupId = appGroupAt(groupIndex)->id();

    if (confManager()->updateApp(appId, appName, endTime, groupId,
                                 useGroupPerm, blocked)) {
        refresh();
        return true;
    }

    return false;
}

bool AppListModel::updateAppName(qint64 appId, const QString &appName)
{
    if (confManager()->updateAppName(appId, appName)) {
        refresh();
        return true;
    }

    return false;
}

void AppListModel::deleteApp(qint64 appId, const QString &appPath, int row)
{
    beginRemoveRows(QModelIndex(), row, row);

    if (confManager()->updateDriverDeleteApp(appPath)
            && confManager()->deleteApp(appId)) {
        invalidateRowCache();
        removeRow(row);
    }

    endRemoveRows();
}

void AppListModel::purgeApps()
{
    for (int row = rowCount(); --row >= 0; ) {
        const auto appRow = appRowAt(row);
        const auto appPath = appRow.appPath;
        if (!FileUtil::fileExists(appPath)) {
            AppInfo appInfo;
            if (!AppUtil::getInfo(appPath, appInfo)) {
                deleteApp(appRow.appId, appPath, row);
            }
        }
    }
}

bool AppListModel::updateTableRow(int row) const
{
    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql().toLatin1(), {row})
          && stmt.step() == SqliteStmt::StepRow))
        return false;

    m_appRow.appId = stmt.columnInt64(0);
    m_appRow.groupIndex = stmt.columnInt(1);
    m_appRow.appPath = stmt.columnText(2);
    m_appRow.appName = stmt.columnText(3);
    m_appRow.useGroupPerm = stmt.columnBool(4);
    m_appRow.blocked = stmt.columnBool(5);
    m_appRow.alerted = stmt.columnBool(6);
    m_appRow.endTime = stmt.columnDateTime(7);
    m_appRow.creatTime = stmt.columnDateTime(8);

    return true;
}

QString AppListModel::sqlBase() const
{
    return
            "SELECT"
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
            "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id"
            ;
}

QString AppListModel::sqlOrderColumn() const
{
    QString columnsStr = "1";
    switch (sortColumn()) {
    case 0: columnsStr = "4 " + sqlOrderAsc() + ", 3"; break;  // Program
    case 1: columnsStr = "2"; break;  // Group
    case 2: columnsStr = "6 " + sqlOrderAsc() + ", 7"; break;  // State
    case 3: columnsStr = "8"; break;  // End Time
    case 4: columnsStr = "1"; break;  // Creation Time
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
