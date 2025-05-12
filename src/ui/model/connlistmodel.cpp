#include "connlistmodel.h"

#include <QFont>
#include <QIcon>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <conf/confrulemanager.h>
#include <conf/confzonemanager.h>
#include <fortmanager.h>
#include <hostinfo/hostinfocache.h>
#include <manager/translationmanager.h>
#include <stat/statconnmanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>

namespace {

const QLoggingCategory LC("connListModel");

QString formatIp(const ip_addr_t ip, bool isIPv6, bool resolveAddress = false)
{
    QString address = NetFormatUtil::ipToText(ip, isIPv6);

    if (resolveAddress) {
        const QString hostName = IoC<HostInfoCache>()->hostName(address);
        if (!hostName.isEmpty()) {
            return hostName;
        }
    }

    if (isIPv6) {
        address = '[' + address + ']';
    }

    return address;
}

QString formatPort(const quint16 port, int role)
{
    if (role == Qt::ToolTipRole) {
        return NetUtil::serviceName(port);
    }

    return QString::number(port);
}

QString reasonIconPath(const ConnRow &connRow)
{
    static const char *const reasonIcons[] = {
        ":/icons/ip.png",
        ":/icons/arrow_refresh_small.png",
        ":/icons/application.png",
        ":/icons/application_double.png",
        ":/icons/lightbulb.png",
        ":/icons/hostname.png",
        ":/icons/ip_class.png",
        ":/icons/script.png",
        ":/icons/script_code.png",
        ":/icons/script_code_red.png",
        ":/icons/help.png",
    };

    if (connRow.reason >= FORT_CONN_REASON_IP_INET
            && connRow.reason <= FORT_CONN_REASON_ASK_LIMIT) {
        const int index = connRow.reason - FORT_CONN_REASON_IP_INET;
        return reasonIcons[index];
    }

    return ":/icons/error.png";
}

QString actionIconPath(const ConnRow &connRow)
{
    return connRow.blocked ? ":/icons/deny.png" : ":/icons/accept.png";
}

QString directionIconPath(const ConnRow &connRow)
{
    return connRow.inbound ? ":/icons/green_down.png" : ":/icons/blue_up.png";
}

QVariant dataDisplayAppName(const ConnRow &connRow, int /*role*/)
{
    return IoC<AppInfoCache>()->appName(connRow.appPath);
}

QVariant dataDisplayProcessId(const ConnRow &connRow, int /*role*/)
{
    return connRow.pid;
}

QVariant dataDisplayProtocolName(const ConnRow &connRow, int /*role*/)
{
    return NetUtil::protocolName(connRow.ipProto);
}

QVariant dataDisplayLocalHostName(const ConnRow &connRow, int role)
{
    const bool resolveAddress = (role >= Qt::UserRole);

    return formatIp(connRow.localIp, connRow.isIPv6, resolveAddress);
}

QVariant dataDisplayLocalIp(const ConnRow &connRow, int /*role*/)
{
    return formatIp(connRow.localIp, connRow.isIPv6);
}

QVariant dataDisplayLocalPort(const ConnRow &connRow, int role)
{
    return formatPort(connRow.localPort, role);
}

QVariant dataDisplayRemoteHostName(const ConnRow &connRow, int role)
{
    const bool resolveAddress = (role >= Qt::UserRole);

    return formatIp(connRow.remoteIp, connRow.isIPv6, resolveAddress);
}

QVariant dataDisplayRemoteIp(const ConnRow &connRow, int /*role*/)
{
    return formatIp(connRow.remoteIp, connRow.isIPv6);
}

QVariant dataDisplayRemotePort(const ConnRow &connRow, int role)
{
    return formatPort(connRow.remotePort, role);
}

QVariant dataDisplayDirection(const ConnRow &connRow, int role)
{
    if (role != Qt::ToolTipRole)
        return {};

    return connRow.inbound ? ConnListModel::tr("In") : ConnListModel::tr("Out");
}

QVariant dataDisplayAction(const ConnRow &connRow, int role)
{
    if (role != Qt::ToolTipRole)
        return {};

    return connRow.blocked ? ConnListModel::tr("Blocked") : ConnListModel::tr("Allowed");
}

QVariant dataDisplayReason(const ConnRow &connRow, int role)
{
    if (role != Qt::ToolTipRole)
        return {};

    QStringList list = { ConnListModel::reasonText(FortConnReason(connRow.reason)) };

    if (connRow.ruleId != 0) {
        const QString ruleName = IoC<ConfRuleManager>()->ruleNameById(connRow.ruleId);

        list << ConnListModel::tr("Rule: %1").arg(ruleName);
    }

    if (connRow.zoneId != 0) {
        const QString zoneName = IoC<ConfZoneManager>()->zoneNameById(connRow.zoneId);

        list << ConnListModel::tr("Zone: %1").arg(zoneName);
    }

    if (connRow.inherited) {
        list << ConnListModel::tr("Inherited");
    }

    return list.join('\n');
}

QVariant dataDisplayTime(const ConnRow &connRow, int /*role*/)
{
    return connRow.connTime;
}

using dataDisplay_func = QVariant (*)(const ConnRow &connRow, int role);

static const dataDisplay_func dataDisplay_funcList[] = {
    &dataDisplayAppName,
    &dataDisplayProcessId,
    &dataDisplayProtocolName,
    &dataDisplayLocalHostName,
    &dataDisplayLocalIp,
    &dataDisplayLocalPort,
    &dataDisplayRemoteHostName,
    &dataDisplayRemoteIp,
    &dataDisplayRemotePort,
    &dataDisplayDirection,
    &dataDisplayAction,
    &dataDisplayReason,
    &dataDisplayTime,
};

}

ConnListModel::ConnListModel(QObject *parent) : TableSqlModel(parent) { }

FortManager *ConnListModel::fortManager() const
{
    return IoC<FortManager>();
}

StatConnManager *ConnListModel::statConnManager() const
{
    return IoC<StatConnManager>();
}

SqliteDb *ConnListModel::sqliteDb() const
{
    return statConnManager()->roSqliteDb();
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
    setSortColumn(int(ConnListColumn::Time));
    setSortOrder(Qt::DescendingOrder);

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &ConnListModel::refresh);
    connect(hostInfoCache(), &HostInfoCache::cacheChanged, this, &ConnListModel::refresh);
    connect(statConnManager(), &StatConnManager::connChanged, this,
            &ConnListModel::updateConnIdRange);

    updateConnIdRange();
}

int ConnListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(ConnListColumn::Count);
}

QVariant ConnListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return headerDataDisplay(section, role);

    // Icon
    case Qt::DecorationRole:
        return headerDataDecoration(section);
    }

    return {};
}

QVariant ConnListModel::data(const QModelIndex &index, int role) const
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
    }

    return {};
}

QVariant ConnListModel::headerDataDisplay(int section, int role) const
{
    if (role == Qt::DisplayRole) {
        if (section >= int(ConnListColumn::Direction) && section <= int(ConnListColumn::Reason))
            return {};
    }

    return ConnListModel::columnName(ConnListColumn(section));
}

QVariant ConnListModel::headerDataDecoration(int section) const
{
    switch (ConnListColumn(section)) {
    case ConnListColumn::Direction:
        return IconCache::icon(":/icons/green_down.png");
    case ConnListColumn::Action:
        return IconCache::icon(":/icons/accept.png");
    case ConnListColumn::Reason:
        return IconCache::icon(":/icons/help.png");
    }

    return {};
}

QVariant ConnListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &connRow = connRowAt(row);
    if (connRow.isNull())
        return {};

    const dataDisplay_func func = dataDisplay_funcList[column];

    if (column == int(ConnListColumn::LocalHostName)
            || column == int(ConnListColumn::RemoteHostName)) {
        role += resolveAddress() ? Qt::UserRole : 0;
    }

    return func(connRow, role);
}

QVariant ConnListModel::dataDecoration(const QModelIndex &index) const
{
    const int column = index.column();
    const int row = index.row();

    const auto &connRow = connRowAt(row);

    switch (ConnListColumn(column)) {
    case ConnListColumn::Program:
        return appInfoCache()->appIcon(connRow.appPath);
    case ConnListColumn::Direction:
        return IconCache::icon(directionIconPath(connRow));
    case ConnListColumn::Action:
        return IconCache::icon(actionIconPath(connRow));
    case ConnListColumn::Reason:
        return IconCache::icon(reasonIconPath(connRow));
    }

    return {};
}

const ConnRow &ConnListModel::connRowAt(int row) const
{
    updateRowCache(row);

    return m_connRow;
}

QString ConnListModel::rowsAsFilter(const QVector<int> &rows) const
{
    QStringList list;

    for (int row : rows) {
        const auto &connRow = connRowAt(row);

        const bool isIPv6 = connRow.isIPv6;

        const auto text = QString("%1:(%2):local_ip(%3):local_port(%4):dir(%5):proto(%6)")
                                  .arg(formatIp(connRow.remoteIp, isIPv6),
                                          QString::number(connRow.remotePort),
                                          formatIp(connRow.localIp, isIPv6),
                                          QString::number(connRow.localPort),
                                          (connRow.inbound ? "IN" : "OUT"),
                                          NetUtil::protocolName(connRow.ipProto));

        list << text;
    }

    return list.join('\n');
}

void ConnListModel::updateConnIdRange()
{
    const qint64 oldIdMin = connIdMin();
    const qint64 oldIdMax = connIdMax();

    qint64 idMin = 0, idMax = 0;
    fillConnIdRange(idMin, idMax);

    updateConnRows(oldIdMin, oldIdMax, idMin, idMax);
}

bool ConnListModel::updateTableRow(const QVariantHash & /*vars*/, int row) const
{
    const qint64 connId = connIdByIndex(row);

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql()).vars({ connId }).prepareRow(stmt))
        return false;

    m_connRow.connId = stmt.columnInt64(0);
    m_connRow.appId = stmt.columnInt64(1);
    m_connRow.connTime = stmt.columnUnixTime(2);
    m_connRow.pid = stmt.columnInt(3);
    m_connRow.reason = stmt.columnInt(4);
    m_connRow.blocked = stmt.columnBool(5);
    m_connRow.inherited = stmt.columnBool(6);
    m_connRow.inbound = stmt.columnBool(7);
    m_connRow.ipProto = stmt.columnInt(8);
    m_connRow.localPort = stmt.columnInt(9);
    m_connRow.remotePort = stmt.columnInt(10);

    m_connRow.isIPv6 = stmt.columnIsNull(11);
    if (!m_connRow.isIPv6) {
        m_connRow.localIp.v4 = stmt.columnInt(11);
        m_connRow.remoteIp.v4 = stmt.columnInt(12);
    } else {
        m_connRow.localIp.v6 = NetUtil::arrayViewToIp6(stmt.columnBlob(13, /*isView=*/true));
        m_connRow.remoteIp.v6 = NetUtil::arrayViewToIp6(stmt.columnBlob(14, /*isView=*/true));
    }

    m_connRow.zoneId = stmt.columnInt(15);
    m_connRow.ruleId = stmt.columnInt(16);

    m_connRow.appPath = stmt.columnText(17);

    return true;
}

void ConnListModel::fillConnIdRange(qint64 &idMin, qint64 &idMax)
{
    statConnManager()->getConnIdRange(sqliteDb(), idMin, idMax);
}

bool ConnListModel::isConnIdRangeOut(
        qint64 oldIdMin, qint64 oldIdMax, qint64 idMin, qint64 idMax) const
{
    const bool isIdMinOut = (idMin < oldIdMin || idMin >= oldIdMax);
    const bool isIdMaxOut = (idMax < oldIdMax || oldIdMax == 0);

    return !(isIdMinOut || isIdMaxOut);
}

qint64 ConnListModel::connIdByIndex(int row) const
{
    return isAscendingOrder() ? (connIdMin() + row) : (connIdMax() - row);
}

int ConnListModel::doSqlCount() const
{
    return connIdMax() <= 0 ? 0 : int(connIdMax() - connIdMin()) + 1;
}

QString ConnListModel::sqlBase() const
{
    return "SELECT"
           "    t.conn_id,"
           "    t.app_id,"
           "    t.conn_time,"
           "    t.process_id,"
           "    t.reason,"
           "    t.blocked,"
           "    t.inherited,"
           "    t.inbound,"
           "    t.ip_proto,"
           "    t.local_port,"
           "    t.remote_port,"
           "    t.local_ip,"
           "    t.remote_ip,"
           "    t.local_ip6,"
           "    t.remote_ip6,"
           "    t.zone_id,"
           "    t.rule_id,"
           "    a.path"
           "  FROM conn t"
           "    JOIN app a ON a.app_id = t.app_id";
}

QString ConnListModel::sqlWhere() const
{
    return " WHERE t.conn_id = ?1";
}

QString ConnListModel::sqlLimitOffset() const
{
    return QString();
}

void ConnListModel::updateConnRows(qint64 oldIdMin, qint64 oldIdMax, qint64 idMin, qint64 idMax)
{
    if (idMin == oldIdMin && idMax == oldIdMax)
        return;

    if (!isConnIdRangeOut(oldIdMin, oldIdMax, idMin, idMax)) {
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

void ConnListModel::resetConnRows(qint64 idMin, qint64 idMax)
{
    setConnIdMin(idMin);
    setConnIdMax(idMax);

    reset();
}

void ConnListModel::removeConnRows(qint64 idMin, int count)
{
    beginRemoveRows({}, 0, count - 1);
    m_connIdMin = idMin;
    invalidateRowCache();
    endRemoveRows();
}

void ConnListModel::insertConnRows(qint64 idMax, int endRow, int count)
{
    beginInsertRows({}, endRow, endRow + count - 1);
    m_connIdMax = idMax;
    invalidateRowCache();
    endInsertRows();
}

QString ConnListModel::reasonText(FortConnReason reason)
{
    static const char *const reasonTexts[] = {
        QT_TR_NOOP("Internet address"),
        QT_TR_NOOP("Old connection"),
        QT_TR_NOOP("Program's action"),
        QT_TR_NOOP("App. Group"),
        QT_TR_NOOP("Filter Mode"),
        QT_TR_NOOP("LAN only"),
        QT_TR_NOOP("Zone"),
        QT_TR_NOOP("Rule"),
        QT_TR_NOOP("Global Rule before App Rules"),
        QT_TR_NOOP("Global Rule after App Rules"),
        QT_TR_NOOP("Limit of Ask to Connect"),
    };

    if (reason >= FORT_CONN_REASON_IP_INET && reason <= FORT_CONN_REASON_ASK_LIMIT) {
        const int index = reason - FORT_CONN_REASON_IP_INET;
        return tr(reasonTexts[index]);
    }

    return tr("Unknown");
}

QString ConnListModel::columnName(const ConnListColumn column)
{
    static QStringList g_columnNames;
    static int g_language = -1;

    const int language = IoC<TranslationManager>()->language();
    if (g_language != language) {
        g_language = language;

        g_columnNames = {
            tr("Program"),
            tr("Process ID"),
            tr("Protocol"),
            tr("Local Host Name"),
            tr("Local IP"),
            tr("Local Port"),
            tr("Remote Host Name"),
            tr("Remote IP"),
            tr("Remote Port"),
            tr("Direction"),
            tr("Action"),
            tr("Reason"),
            tr("Time"),
        };
    }

    return g_columnNames.value(int(column));
}
