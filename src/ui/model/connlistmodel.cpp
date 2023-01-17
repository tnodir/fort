#include "connlistmodel.h"

#include <QFont>
#include <QIcon>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <fortmanager.h>
#include <hostinfo/hostinfocache.h>
#include <log/logentryblockedip.h>
#include <stat/statmanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netutil.h>

ConnListModel::ConnListModel(QObject *parent) :
    TableSqlModel(parent), m_connMode(ConnNone), m_resolveAddress(false)
{
}

void ConnListModel::setConnMode(uint v)
{
    if (m_connMode != v) {
        m_connMode = v;
        resetRowIdRange();
    }
}

void ConnListModel::setResolveAddress(bool v)
{
    if (m_resolveAddress != v) {
        m_resolveAddress = v;
        refresh();
    }
}

FortManager *ConnListModel::fortManager() const
{
    return IoC<FortManager>();
}

StatManager *ConnListModel::statManager() const
{
    return IoC<StatManager>();
}

SqliteDb *ConnListModel::sqliteDb() const
{
    return statManager()->sqliteDb();
}

AppInfoCache *ConnListModel::appInfoCache() const
{
    return IoC<AppInfoCache>();
}

HostInfoCache *ConnListModel::hostInfoCache() const
{
    return IoC<HostInfoCache>();
}

void ConnListModel::initialize()
{
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &ConnListModel::refresh);
    connect(hostInfoCache(), &HostInfoCache::cacheChanged, this, &ConnListModel::refresh);
    connect(statManager(), &StatManager::connChanged, this, &ConnListModel::updateRowIdRange);
}

int ConnListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 7;
}

QVariant ConnListModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant ConnListModel::data(const QModelIndex &index, int role) const
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

QVariant ConnListModel::dataDisplay(const QModelIndex &index, int role) const
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

QVariant ConnListModel::dataDisplayDirection(const ConnRow &connRow, int role) const
{
    if (role == Qt::ToolTipRole) {
        if (connRow.blocked) {
            // Show block reason in a tool-tip
            return blockReasonText(connRow)
                    + (connRow.inherited ? " (" + tr("Inherited") + ")" : QString());
        }
    }
    return connRow.inbound ? tr("In") : tr("Out");
}

QVariant ConnListModel::dataDecoration(const QModelIndex &index) const
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

QString ConnListModel::blockReasonText(const ConnRow &connRow)
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
    default:
        return tr("Unknown");
    }
}

QString ConnListModel::connIconPath(const ConnRow &connRow)
{
    if (connRow.blocked) {
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
        default:
            return ":/icons/deny.png";
        }
    }

    return ":/icons/accept.png";
}

void ConnListModel::deleteConn(qint64 rowIdTo, bool blocked)
{
    statManager()->deleteConn(rowIdTo, blocked);
}

const ConnRow &ConnListModel::connRowAt(int row) const
{
    updateRowCache(row);

    return m_connRow;
}

void ConnListModel::clear()
{
    statManager()->deleteConnAll();

    hostInfoCache()->clear();
}

void ConnListModel::resetRowIdRange()
{
    m_rowIdMin = 0, m_rowIdMax = 0;
    updateRowIdRange();
}

void ConnListModel::updateRowIdRange()
{
    if (connMode() == ConnNone)
        return;

    const qint64 oldIdMin = rowIdMin();
    const qint64 oldIdMax = rowIdMax();

    qint64 idMin, idMax;
    getRowIdRange(idMin, idMax);

    if (idMin == oldIdMin && idMax == oldIdMax)
        return;

    if (idMin < oldIdMin || idMax < oldIdMax || oldIdMax == 0) {
        m_rowIdMin = idMin, m_rowIdMax = idMax;
        reset();
        return;
    }

    if (idMin > oldIdMin) {
        const int removedCount = idMin - oldIdMin;
        beginRemoveRows({}, 0, removedCount - 1);
        m_rowIdMin = idMin;
        invalidateRowCache();
        endRemoveRows();
    }

    if (idMax > oldIdMax) {
        const int addedCount = idMax - oldIdMax;
        const int endRow = oldIdMax - idMin + 1;
        beginInsertRows({}, endRow, endRow + addedCount - 1);
        m_rowIdMax = idMax;
        invalidateRowCache();
        endInsertRows();
    }
}

void ConnListModel::getRowIdRange(qint64 &rowIdMin, qint64 &rowIdMax) const
{
    statManager()->updateConnBlockId();

    rowIdMin = isConnBlock() ? statManager()->connBlockIdMin() : statManager()->connTrafIdMin();
    rowIdMax = isConnBlock() ? statManager()->connBlockIdMax() : statManager()->connTrafIdMax();
}

bool ConnListModel::updateTableRow(int row) const
{
    const qint64 rowId = rowIdMin() + row;

    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql(), { rowId }) && stmt.step() == SqliteStmt::StepRow))
        return false;

    m_connRow.rowId = rowId;
    m_connRow.connId = stmt.columnInt64(0);
    m_connRow.appId = stmt.columnInt64(1);
    m_connRow.connTime = stmt.columnUnixTime(2);
    m_connRow.pid = stmt.columnInt(3);
    m_connRow.inbound = stmt.columnBool(4);
    m_connRow.inherited = stmt.columnBool(5);
    m_connRow.blocked = stmt.columnBool(6);
    m_connRow.ipProto = stmt.columnInt(7);
    m_connRow.localPort = stmt.columnInt(8);
    m_connRow.remotePort = stmt.columnInt(9);

    m_connRow.isIPv6 = stmt.columnIsNull(10);
    if (m_connRow.isIPv6) {
        m_connRow.localIp.v6 = NetUtil::rawArrayToIp6(stmt.columnBlob(11, /*isRaw=*/true));
        m_connRow.remoteIp.v6 = NetUtil::rawArrayToIp6(stmt.columnBlob(13, /*isRaw=*/true));
    } else {
        m_connRow.localIp.v4 = stmt.columnInt(10);
        m_connRow.remoteIp.v4 = stmt.columnInt(12);
    }

    m_connRow.appPath = stmt.columnText(14);

    if (isConnBlock()) {
        m_connRow.blockReason = stmt.columnInt(15);
    }

    return true;
}

int ConnListModel::doSqlCount() const
{
    return rowIdMax() <= 0 ? 0 : int(rowIdMax() - rowIdMin()) + 1;
}

QString ConnListModel::sqlBase() const
{
    return QString::fromLatin1("SELECT"
                               "    t.conn_id,"
                               "    t.app_id,"
                               "    t.conn_time,"
                               "    t.process_id,"
                               "    t.inbound,"
                               "    t.inherited,"
                               "    t.blocked,"
                               "    t.ip_proto,"
                               "    t.local_port,"
                               "    t.remote_port,"
                               "    t.local_ip,"
                               "    t.local_ip6,"
                               "    t.remote_ip,"
                               "    t.remote_ip6,"
                               "    a.path,"
                               "    %2"
                               "  FROM conn t"
                               "    JOIN %1 c ON c.conn_id = t.conn_id"
                               "    JOIN app a ON a.app_id = t.app_id")
            .arg(isConnBlock() ? "conn_block" : "conn_traffic",
                    isConnBlock() ? "c.block_reason" : "c.end_time, c.in_bytes, c.out_bytes");
}

QString ConnListModel::sqlWhere() const
{
    return " WHERE c.id = ?1";
}

QString ConnListModel::sqlLimitOffset() const
{
    return QString();
}

QString ConnListModel::formatIpPort(const ip_addr_t &ip, quint16 port, bool isIPv6) const
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
