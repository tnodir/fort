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

    quint8 blockReason = 0;

    quint8 ipProto = 0;
    quint16 localPort = 0;
    quint16 remotePort = 0;
    quint32 localIp = 0;
    quint32 remoteIp = 0;

    quint32 pid = 0;

    qint64 rowId = 0;
    qint64 connId = 0;
    qint64 appId = 0;

    QString appPath;

    QDateTime connTime;
};

class ConnListModel : public TableSqlModel
{
    Q_OBJECT

public:
    enum ConnMode : qint8 { ConnNone = 0, ConnBlock, ConnTraf };

    explicit ConnListModel(StatManager *statManager, QObject *parent = nullptr);

    uint connMode() const { return m_connMode; }
    void setConnMode(uint v);

    bool isConnBlock() const { return connMode() == ConnBlock; }

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

    void deleteConn(qint64 rowIdTo, bool blocked);

    const ConnRow &connRowAt(int row) const;

public slots:
    void clear();

protected slots:
    void resetRowIdRange();
    void updateRowIdRange();

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_connRow; }

    int doSqlCount() const override;
    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlLimitOffset() const override;

private:
    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDisplayDirection(const ConnRow &connRow, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;

    static QString blockReasonText(const ConnRow &connRow);
    static QString connIconPath(const ConnRow &connRow);

    qint64 rowIdMin() const { return m_rowIdMin; }
    qint64 rowIdMax() const { return m_rowIdMax; }

    void getRowIdRange(qint64 &rowIdMin, qint64 &rowIdMax) const;

    QString formatIpPort(quint32 ip, quint16 port) const;

private:
    uint m_connMode : 2;
    uint m_resolveAddress : 1;

    qint64 m_rowIdMin = 0;
    qint64 m_rowIdMax = 0;

    StatManager *m_statManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;
    HostInfoCache *m_hostInfoCache = nullptr;

    mutable ConnRow m_connRow;
};

#endif // CONNLISTMODEL_H
