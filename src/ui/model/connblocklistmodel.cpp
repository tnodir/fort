#include "connblocklistmodel.h"

#include <QFont>
#include <QIcon>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <fortmanager.h>
#include <hostinfo/hostinfocache.h>
#include <log/logentryblockedip.h>
#include <stat/statblockmanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netutil.h>

ConnBlockListModel::ConnBlockListModel(QObject *parent) :
    TableSqlModel(parent), m_resolveAddress(false)
{
}

void ConnBlockListModel::setResolveAddress(bool v)
{
    if (m_resolveAddress != v) {
        m_resolveAddress = v;
        refresh();
    }
}

FortManager *ConnBlockListModel::fortManager() const
{
    return IoC<FortManager>();
}

StatBlockManager *ConnBlockListModel::statBlockManager() const
{
    return IoC<StatBlockManager>();
}

SqliteDb *ConnBlockListModel::sqliteDb() const
{
    return statBlockManager()->roSqliteDb();
}

AppInfoCache *ConnBlockListModel::appInfoCache() const
{
    return IoC<AppInfoCache>();
}

HostInfoCache *ConnBlockListModel::hostInfoCache() const
{
    return IoC<HostInfoCache>();
}

void ConnBlockListModel::initialize()
{
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &ConnBlockListModel::refresh);
    connect(hostInfoCache(), &HostInfoCache::cacheChanged, this, &ConnBlockListModel::refresh);
    connect(statBlockManager(), &StatBlockManager::connChanged, this,
            &ConnBlockListModel::updateConnIdRange);

    updateConnIdRange();
}

int ConnBlockListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 7;
}

QVariant ConnBlockListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        switch (section) {
        case 0:
            return tr("Program");
        case 1:
            return (role == Qt::DisplayRole) ? tr("Proc. ID") : tr("Process ID");
        case 2:
            return tr("Protocol");
        case 3:
            return tr("Local IP and Port");
        case 4:
            return tr("Remote IP and Port");
        case 5:
            return (role == Qt::DisplayRole) ? tr("Dir.") : tr("Direction");
        case 6:
            return tr("Time");
        }
    }
    return QVariant();
}

QVariant ConnBlockListModel::data(const QModelIndex &index, int role) const
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
    }

    return QVariant();
}

QVariant ConnBlockListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();
    const int column = index.column();

    const auto connRow = connRowAt(row);

    switch (column) {
    case 0:
        return appInfoCache()->appName(connRow.appPath);
    case 1:
        return connRow.pid;
    case 2:
        return NetUtil::protocolName(connRow.ipProto);
    case 3:
        return formatIpPort(connRow.localIp, connRow.localPort, connRow.isIPv6);
    case 4:
        return formatIpPort(connRow.remoteIp, connRow.remotePort, connRow.isIPv6);
    case 5:
        return dataDisplayDirection(connRow, role);
    case 6:
        return connRow.connTime;
    }

    return QVariant();
}

QVariant ConnBlockListModel::dataDisplayDirection(const ConnRow &connRow, int role) const
{
    if (role == Qt::ToolTipRole) {
        // Show block reason in a tool-tip
        return blockReasonText(connRow)
                + (connRow.inherited ? " (" + tr("Inherited") + ")" : QString());
    }
    return connRow.inbound ? tr("In") : tr("Out");
}

QVariant ConnBlockListModel::dataDecoration(const QModelIndex &index) const
{
    const int column = index.column();

    if (column == 0 || column == 5) {
        const int row = index.row();
        const auto connRow = connRowAt(row);

        switch (column) {
        case 0:
            return appInfoCache()->appIcon(connRow.appPath);
        case 5:
            return IconCache::icon(connIconPath(connRow));
        }
    }

    return QVariant();
}

QString ConnBlockListModel::blockReasonText(const ConnRow &connRow)
{
    switch (connRow.blockReason) {
    case FORT_BLOCK_REASON_IP_INET:
        return tr("Blocked Internet address");
    case FORT_BLOCK_REASON_REAUTH:
        return tr("Old connection closed on startup");
    case FORT_BLOCK_REASON_PROGRAM:
        return tr("Programs logic");
    case FORT_BLOCK_REASON_APP_GROUP_FOUND:
        return tr("App. Group logic");
    case FORT_BLOCK_REASON_FILTER_MODE:
        return tr("Filter Mode logic");
    case FORT_BLOCK_REASON_LAN_ONLY:
        return tr("Restrict access to LAN only");
    default:
        return tr("Unknown");
    }
}

QString ConnBlockListModel::connIconPath(const ConnRow &connRow)
{
    switch (connRow.blockReason) {
    case FORT_BLOCK_REASON_IP_INET:
        return ":/icons/ip.png";
    case FORT_BLOCK_REASON_REAUTH:
        return ":/icons/arrow_refresh_small.png";
    case FORT_BLOCK_REASON_PROGRAM:
        return ":/icons/application.png";
    case FORT_BLOCK_REASON_APP_GROUP_FOUND:
        return ":/icons/application_double.png";
    case FORT_BLOCK_REASON_FILTER_MODE:
        return ":/icons/deny.png";
    case FORT_BLOCK_REASON_LAN_ONLY:
        return ":/icons/hostname.png";
    default:
        return ":/icons/error.png";
    }
}

void ConnBlockListModel::deleteConn(qint64 connIdTo)
{
    statBlockManager()->deleteConn(connIdTo);
}

const ConnRow &ConnBlockListModel::connRowAt(int row) const
{
    updateRowCache(row);

    return m_connRow;
}

void ConnBlockListModel::clear()
{
    statBlockManager()->deleteConn();

    hostInfoCache()->clear();
}

void ConnBlockListModel::updateConnIdRange()
{
    const qint64 oldIdMin = connIdMin();
    const qint64 oldIdMax = connIdMax();

    qint64 idMin, idMax;
    statBlockManager()->getConnIdRange(sqliteDb(), idMin, idMax);

    if (idMin == oldIdMin && idMax == oldIdMax)
        return;

    if (idMin < oldIdMin || idMin >= oldIdMax || idMax < oldIdMax || oldIdMax == 0) {
        m_connIdMin = idMin, m_connIdMax = idMax;
        reset();
        return;
    }

    if (idMin > oldIdMin) {
        const int removedCount = idMin - oldIdMin;
        beginRemoveRows({}, 0, removedCount - 1);
        m_connIdMin = idMin;
        invalidateRowCache();
        endRemoveRows();
    }

    if (idMax > oldIdMax) {
        const int addedCount = idMax - oldIdMax;
        const int endRow = oldIdMax - idMin + 1;
        beginInsertRows({}, endRow, endRow + addedCount - 1);
        m_connIdMax = idMax;
        invalidateRowCache();
        endInsertRows();
    }
}

bool ConnBlockListModel::updateTableRow(int row) const
{
    const qint64 connId = connIdMin() + row;

    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql(), { connId }) && stmt.step() == SqliteStmt::StepRow))
        return false;

    m_connRow.connId = stmt.columnInt64(0);
    m_connRow.appId = stmt.columnInt64(1);
    m_connRow.connTime = stmt.columnUnixTime(2);
    m_connRow.pid = stmt.columnInt(3);
    m_connRow.inbound = stmt.columnBool(4);
    m_connRow.inherited = stmt.columnBool(5);
    m_connRow.ipProto = stmt.columnInt(6);
    m_connRow.localPort = stmt.columnInt(7);
    m_connRow.remotePort = stmt.columnInt(8);

    m_connRow.isIPv6 = stmt.columnIsNull(9);
    if (!m_connRow.isIPv6) {
        m_connRow.localIp.v4 = stmt.columnInt(9);
        m_connRow.remoteIp.v4 = stmt.columnInt(10);
    } else {
        m_connRow.localIp.v6 = NetUtil::rawArrayToIp6(stmt.columnBlob(11, /*isRaw=*/true));
        m_connRow.remoteIp.v6 = NetUtil::rawArrayToIp6(stmt.columnBlob(12, /*isRaw=*/true));
    }

    m_connRow.blockReason = stmt.columnInt(13);

    m_connRow.appPath = stmt.columnText(14);

    return true;
}

int ConnBlockListModel::doSqlCount() const
{
    return connIdMax() <= 0 ? 0 : int(connIdMax() - connIdMin()) + 1;
}

QString ConnBlockListModel::sqlBase() const
{
    return "SELECT"
           "    t.conn_id,"
           "    t.app_id,"
           "    t.conn_time,"
           "    t.process_id,"
           "    t.inbound,"
           "    t.inherited,"
           "    t.ip_proto,"
           "    t.local_port,"
           "    t.remote_port,"
           "    t.local_ip,"
           "    t.remote_ip,"
           "    t.local_ip6,"
           "    t.remote_ip6,"
           "    t.block_reason,"
           "    a.path"
           "  FROM conn_block t"
           "    JOIN app a ON a.app_id = t.app_id";
}

QString ConnBlockListModel::sqlWhere() const
{
    return " WHERE t.conn_id = ?1";
}

QString ConnBlockListModel::sqlLimitOffset() const
{
    return QString();
}

QString ConnBlockListModel::formatIpPort(const ip_addr_t &ip, quint16 port, bool isIPv6) const
{
    QString address = NetUtil::ipToText(ip, isIPv6);
    if (resolveAddress()) {
        const QString hostName = hostInfoCache()->hostName(address);
        if (!hostName.isEmpty()) {
            address = hostName;
        }
    }
    if (isIPv6) {
        address = '[' + address + ']';
    }
    return address + ':' + QString::number(port);
}
