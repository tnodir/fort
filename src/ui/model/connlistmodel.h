#ifndef CONNLISTMODEL_H
#define CONNLISTMODEL_H

#include <QDateTime>

#include "../util/model/tablesqlmodel.h"

class AppInfoCache;
class FortManager;
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

    explicit ConnListModel(FortManager *fortManager, QObject *parent = nullptr);

    uint connMode() const { return m_connMode; }
    void setConnMode(uint v);

    bool isConnBlock() const { return connMode() == ConnBlock; }

    bool resolveAddress() const { return m_resolveAddress; }
    void setResolveAddress(bool v);

    FortManager *fortManager() const { return m_fortManager; }
    StatManager *statManager() const;
    SqliteDb *sqliteDb() const override;
    AppInfoCache *appInfoCache() const;
    HostInfoCache *hostInfoCache() const;

    void initialize();

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

    FortManager *m_fortManager = nullptr;

    mutable ConnRow m_connRow;
};

#endif // CONNLISTMODEL_H
