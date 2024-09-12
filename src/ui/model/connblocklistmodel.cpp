#include "connblocklistmodel.h"

#include <QFont>
#include <QIcon>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
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

namespace {

const QLoggingCategory LC("connBlockListModel");

QString formatIpPort(const ip_addr_t &ip, quint16 port, bool isIPv6, bool resolveAddress)
{
    QString address = NetUtil::ipToText(ip, isIPv6);
    if (resolveAddress) {
        const QString hostName = IoC<HostInfoCache>()->hostName(address);
        if (!hostName.isEmpty()) {
            address = hostName;
        }
    }
    if (isIPv6) {
        address = '[' + address + ']';
    }
    return address + ':' + QString::number(port);
}

QString reasonText(const ConnRow &connRow)
{
    static const char *const blockReasonTexts[] = {
        QT_TR_NOOP("Blocked Internet address"),
        QT_TR_NOOP("Old connection closed on startup"),
        QT_TR_NOOP("Programs logic"),
        QT_TR_NOOP("App. Group logic"),
        QT_TR_NOOP("Filter Mode logic"),
        QT_TR_NOOP("Restrict access to LAN only"),
        QT_TR_NOOP("Restrict access by Zone"),
        QT_TR_NOOP("Limit of Ask to Connect"),
    };

    if (connRow.blockReason >= FORT_BLOCK_REASON_IP_INET
            && connRow.blockReason <= FORT_BLOCK_REASON_ASK_LIMIT) {
        const int index = connRow.blockReason - FORT_BLOCK_REASON_IP_INET;
        return ConnBlockListModel::tr(blockReasonTexts[index]);
    }

    return ConnBlockListModel::tr("Unknown");
}

QString reasonIconPath(const ConnRow &connRow)
{
    static const char *const blockReasonIcons[] = {
        ":/icons/ip.png",
        ":/icons/arrow_refresh_small.png",
        ":/icons/application.png",
        ":/icons/application_double.png",
        ":/icons/deny.png",
        ":/icons/hostname.png",
        ":/icons/ip_class.png",
        ":/icons/help.png",
    };

    if (connRow.blockReason >= FORT_BLOCK_REASON_IP_INET
            && connRow.blockReason <= FORT_BLOCK_REASON_ASK_LIMIT) {
        const int index = connRow.blockReason - FORT_BLOCK_REASON_IP_INET;
        return blockReasonIcons[index];
    }

    return ":/icons/error.png";
}

QString directionIconPath(const ConnRow &connRow)
{
    return connRow.inbound ? ":/icons/green_down.png" : ":/icons/blue_up.png";
}

QVariant dataDisplayAppName(const ConnRow &connRow, bool /*resolveAddress*/, int /*role*/)
{
    return IoC<AppInfoCache>()->appName(connRow.appPath);
}

QVariant dataDisplayProcessId(const ConnRow &connRow, bool /*resolveAddress*/, int /*role*/)
{
    return connRow.pid;
}

QVariant dataDisplayProtocolName(const ConnRow &connRow, bool /*resolveAddress*/, int /*role*/)
{
    return NetUtil::protocolName(connRow.ipProto);
}

QVariant dataDisplayLocalIpPort(const ConnRow &connRow, bool resolveAddress, int /*role*/)
{
    return formatIpPort(connRow.localIp, connRow.localPort, connRow.isIPv6, resolveAddress);
}

QVariant dataDisplayRemoteIpPort(const ConnRow &connRow, bool resolveAddress, int /*role*/)
{
    return formatIpPort(connRow.remoteIp, connRow.remotePort, connRow.isIPv6, resolveAddress);
}

QVariant dataDisplayDirection(const ConnRow &connRow, bool /*resolveAddress*/, int role)
{
    if (role == Qt::ToolTipRole) {
        return connRow.inbound ? ConnBlockListModel::tr("In") : ConnBlockListModel::tr("Out");
    }

    return QVariant();
}

QVariant dataDisplayReason(const ConnRow &connRow, bool /*resolveAddress*/, int role)
{
    if (role == Qt::ToolTipRole) {
        return reasonText(connRow)
                + (connRow.inherited ? " (" + ConnBlockListModel::tr("Inherited") + ")"
                                     : QString());
    }

    return QVariant();
}

QVariant dataDisplayTime(const ConnRow &connRow, bool /*resolveAddress*/, int /*role*/)
{
    return connRow.connTime;
}

using dataDisplay_func = QVariant (*)(const ConnRow &connRow, bool resolveAddress, int role);

static const dataDisplay_func dataDisplay_funcList[] = {
    &dataDisplayAppName,
    &dataDisplayProcessId,
    &dataDisplayProtocolName,
    &dataDisplayLocalIpPort,
    &dataDisplayRemoteIpPort,
    &dataDisplayDirection,
    &dataDisplayReason,
    &dataDisplayTime,
};

}

ConnBlockListModel::ConnBlockListModel(QObject *parent) : TableSqlModel(parent) { }

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

int ConnBlockListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 8;
}

QVariant ConnBlockListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return headerDataDisplay(section, role);

    // Icon
    case Qt::DecorationRole:
        return headerDataDecoration(section);
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

QVariant ConnBlockListModel::headerDataDisplay(int section, int role) const
{
    static const char *const headerTexts[] = {
        QT_TR_NOOP("Program"),
        QT_TR_NOOP("Proc. ID"),
        QT_TR_NOOP("Protocol"),
        QT_TR_NOOP("Local IP and Port"),
        QT_TR_NOOP("Remote IP and Port"),
        nullptr,
        nullptr,
        QT_TR_NOOP("Time"),
    };

    static const char *const headerTooltips[] = {
        QT_TR_NOOP("Program"),
        QT_TR_NOOP("Process ID"),
        QT_TR_NOOP("Protocol"),
        QT_TR_NOOP("Local IP and Port"),
        QT_TR_NOOP("Remote IP and Port"),
        QT_TR_NOOP("Direction"),
        QT_TR_NOOP("Reason"),
        QT_TR_NOOP("Time"),
    };

    if (section >= 0 && section <= 7) {
        const char *const *arr = (role == Qt::ToolTipRole) ? headerTooltips : headerTexts;
        const char *text = arr[section];

        if (text != nullptr) {
            return tr(text);
        }
    }

    return QVariant();
}

QVariant ConnBlockListModel::headerDataDecoration(int section) const
{
    switch (section) {
    case 5:
        return IconCache::icon(":/icons/green_down.png");
    case 6:
        return IconCache::icon(":/icons/help.png");
    }

    return QVariant();
}

QVariant ConnBlockListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &connRow = connRowAt(row);
    if (connRow.isNull())
        return {};

    const dataDisplay_func func = dataDisplay_funcList[column];

    return func(connRow, resolveAddress(), role);
}

QVariant ConnBlockListModel::dataDecoration(const QModelIndex &index) const
{
    const int column = index.column();
    const int row = index.row();

    const auto &connRow = connRowAt(row);

    switch (column) {
    case 0:
        return appInfoCache()->appIcon(connRow.appPath);
    case 5:
        return IconCache::icon(directionIconPath(connRow));
    case 6:
        return IconCache::icon(reasonIconPath(connRow));
    }

    return QVariant();
}

const ConnRow &ConnBlockListModel::connRowAt(int row) const
{
    updateRowCache(row);

    return m_connRow;
}

void ConnBlockListModel::updateConnIdRange()
{
    const qint64 oldIdMin = connIdMin();
    const qint64 oldIdMax = connIdMax();

    qint64 idMin, idMax;
    statBlockManager()->getConnIdRange(sqliteDb(), idMin, idMax);

    if (idMin == oldIdMin && idMax == oldIdMax)
        return;

    if (idMax == 0) {
        hostInfoCache()->clear();
    }

    updateConnRows(oldIdMin, oldIdMax, idMin, idMax);
}

bool ConnBlockListModel::updateTableRow(const QVariantHash & /*vars*/, int row) const
{
    const qint64 connId = connIdMin() + row;

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql()).vars({ connId }).prepareRow(stmt))
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

void ConnBlockListModel::updateConnRows(
        qint64 oldIdMin, qint64 oldIdMax, qint64 idMin, qint64 idMax)
{
    const bool isIdMinOut = (idMin < oldIdMin || idMin >= oldIdMax);
    const bool isIdMaxOut = (idMax < oldIdMax || oldIdMax == 0);

    if (isIdMinOut || isIdMaxOut) {
        resetConnRows(idMin, idMax);
        return;
    }

    const int removedCount = idMin - oldIdMin;
    if (removedCount > 0) {
        removeConnRows(idMin, removedCount);
    }

    const int addedCount = idMax - oldIdMax;
    if (addedCount > 0) {
        const int endRow = oldIdMax - idMin + 1;
        insertConnRows(idMax, endRow, addedCount);
    }
}

void ConnBlockListModel::resetConnRows(qint64 idMin, qint64 idMax)
{
    m_connIdMin = idMin;
    m_connIdMax = idMax;
    reset();
}

void ConnBlockListModel::removeConnRows(qint64 idMin, int count)
{
    beginRemoveRows({}, 0, count - 1);
    m_connIdMin = idMin;
    invalidateRowCache();
    endRemoveRows();
}

void ConnBlockListModel::insertConnRows(qint64 idMax, int endRow, int count)
{
    beginInsertRows({}, endRow, endRow + count - 1);
    m_connIdMax = idMax;
    invalidateRowCache();
    endInsertRows();
}
