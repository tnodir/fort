#include "connlistmodel.h"

#include <QFont>
#include <QIcon>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../appinfo/appinfocache.h"
#include "../log/logentryblockedip.h"
#include "../stat/statmanager.h"
#include "../util/iconcache.h"
#include "../util/net/hostinfocache.h"
#include "../util/net/netutil.h"

ConnListModel::ConnListModel(StatManager *statManager, QObject *parent) :
    TableSqlModel(parent), m_statManager(statManager)
{
    connect(m_statManager, &StatManager::connBlockAdded, this, &ConnListModel::reset);
    connect(m_statManager, &StatManager::connRemoved, this, &ConnListModel::reset);
}

void ConnListModel::setBlockedMode(bool v)
{
    if (m_blockedMode != v) {
        m_blockedMode = v;
        reset();
    }
}

void ConnListModel::setResolveAddress(bool v)
{
    if (m_resolveAddress != v) {
        m_resolveAddress = v;
        refresh();
    }
}

SqliteDb *ConnListModel::sqliteDb() const
{
    return statManager()->sqliteDb();
}

void ConnListModel::setAppInfoCache(AppInfoCache *v)
{
    m_appInfoCache = v;

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &ConnListModel::refresh);
}

void ConnListModel::setHostInfoCache(HostInfoCache *v)
{
    m_hostInfoCache = v;

    connect(hostInfoCache(), &HostInfoCache::cacheChanged, this, &ConnListModel::refresh);
}

void ConnListModel::handleLogBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime)
{
    const int row = rowCount();

    doBeginInsertRows(row, row);

    statManager()->logBlockedIp(entry, unixTime);

    doEndInsertRows();
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

    // Font
    case Qt::FontRole:
        return dataFont(index);
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
        return formatIpPort(connRow.localIp, connRow.localPort);
    case 4:
        return formatIpPort(connRow.remoteIp, connRow.remotePort);
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
            // Show block reason in tool-tip
            const auto blockRow = getConnRowBlock(connRow.rowId);
            return blockReasonText(blockRow);
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
            return connRow.blocked ? IconCache::icon(":/icons/sign-ban.png")
                                   : IconCache::icon(":/icons/sign-check.png");
        }
    }

    return QVariant();
}

QVariant ConnListModel::dataFont(const QModelIndex &index) const
{
    const int column = index.column();

    switch (column) {
    case 6: {
        QFont font;
        font.setPointSize(8);
        return font;
    }
    }

    return QVariant();
}

QString ConnListModel::blockReasonText(const ConnBlockRow &blockRow)
{
    switch (blockRow.blockReason) {
    case LogEntryBlockedIp::ReasonIpInet:
        return tr("Blocked Internet address");
    case LogEntryBlockedIp::ReasonReauth:
        return tr("Old connection closed on startup");
    case LogEntryBlockedIp::ReasonProgram:
        return tr("Programs logic");
    case LogEntryBlockedIp::ReasonAppGroupFound:
        return tr("App. Group logic");
    case LogEntryBlockedIp::ReasonAppGroupDefault:
        return tr("App. Group default logic");
    default:
        return tr("Unknown");
    }
}

void ConnListModel::deleteConn(qint64 rowIdTo, bool blocked, int row)
{
    doBeginRemoveRows(0, row);

    statManager()->deleteConn(rowIdTo, blocked);

    doEndRemoveRows();
}

const ConnRow &ConnListModel::connRowAt(int row) const
{
    updateRowCache(row);

    return m_connRow;
}

ConnBlockRow ConnListModel::getConnRowBlock(qint64 rowId) const
{
    static const char *const sql = "SELECT block_reason FROM conn_block WHERE id = ?1";

    return { quint8(sqliteDb()->executeEx(sql, { rowId }).toInt()) };
}

void ConnListModel::clear()
{
    statManager()->deleteConnAll();

    hostInfoCache()->clear();
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
    m_connRow.blocked = stmt.columnBool(5);
    m_connRow.ipProto = stmt.columnInt(6);
    m_connRow.localPort = stmt.columnInt(7);
    m_connRow.remotePort = stmt.columnInt(8);
    m_connRow.localIp = stmt.columnInt(9);
    m_connRow.remoteIp = stmt.columnInt(10);
    m_connRow.appPath = stmt.columnText(11);

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
                               "    t.blocked,"
                               "    t.ip_proto,"
                               "    t.local_port,"
                               "    t.remote_port,"
                               "    t.local_ip,"
                               "    t.remote_ip,"
                               "    a.path"
                               "  FROM conn t"
                               "    JOIN %1 c ON c.conn_id = t.conn_id"
                               "    JOIN app a ON a.app_id = t.app_id")
            .arg(blockedMode() ? "conn_block" : "conn_traffic");
}

QString ConnListModel::sqlWhere() const
{
    return " WHERE c.id = ?1";
}

QString ConnListModel::sqlLimitOffset() const
{
    return QString();
}

qint64 ConnListModel::rowIdMin() const
{
    return blockedMode() ? statManager()->connBlockIdMin() : statManager()->connTrafIdMin();
}

qint64 ConnListModel::rowIdMax() const
{
    return blockedMode() ? statManager()->connBlockIdMax() : statManager()->connTrafIdMax();
}

QString ConnListModel::formatIpPort(quint32 ip, quint16 port) const
{
    QString address = NetUtil::ip4ToText(ip);
    if (resolveAddress()) {
        const QString hostName = hostInfoCache()->hostName(address);
        if (!hostName.isEmpty()) {
            address = hostName;
        }
    }
    return address + ':' + QString::number(port);
}
