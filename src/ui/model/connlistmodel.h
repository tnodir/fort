#ifndef CONNLISTMODEL_H
#define CONNLISTMODEL_H

#include <QDateTime>

#include "../util/model/tablesqlmodel.h"

class AppInfoCache;
class HostInfoCache;
class LogEntryBlockedIp;
class StatManager;

struct ConnRow : TableRow
{
    bool inbound = false;
    bool blocked = false;

    quint8 ipProto = 0;
    quint16 localPort = 0;
    quint16 remotePort = 0;
    quint32 localIp = 0;
    quint32 remoteIp = 0;

    quint32 pid = 0;

    qint64 connId = 0;
    qint64 appId = 0;

    QString appPath;

    QDateTime connTime;
};

struct ConnRowBlock
{
    quint8 blockReason = 0;
};

class ConnListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit ConnListModel(StatManager *statManager, QObject *parent = nullptr);

    bool resolveAddress() const { return m_resolveAddress; }
    void setResolveAddress(bool v);

    StatManager *statManager() const { return m_statManager; }
    SqliteDb *sqliteDb() const override;

    AppInfoCache *appInfoCache() const { return m_appInfoCache; }
    void setAppInfoCache(AppInfoCache *v);

    HostInfoCache *hostInfoCache() const { return m_hostInfoCache; }
    void setHostInfoCache(HostInfoCache *v);

    void handleLogBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void deleteConn(qint64 connId, bool blocked, int row);

    const ConnRow &connRowAt(int row) const;

    ConnRowBlock getConnRowBlock(qint64 connId) const;

public slots:
    void clear();

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_connRow; }

    QString sqlBase() const override;

private:
    QString formatIpPort(quint32 ip, quint16 port) const;

private:
    bool m_resolveAddress = false;

    int m_connBlockInc = 999999999; // to trigger on first check

    StatManager *m_statManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;
    HostInfoCache *m_hostInfoCache = nullptr;

    mutable ConnRow m_connRow;
};

#endif // CONNLISTMODEL_H
