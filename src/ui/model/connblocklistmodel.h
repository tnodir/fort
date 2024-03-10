#ifndef CONNBLOCKLISTMODEL_H
#define CONNBLOCKLISTMODEL_H

#include <QDateTime>

#include <common/common_types.h>
#include <util/model/tablesqlmodel.h>

class AppInfoCache;
class FortManager;
class HostInfoCache;
class StatBlockManager;

struct ConnRow : TableRow
{
    bool isIPv6 : 1 = false;
    bool inbound : 1 = false;
    bool inherited : 1 = false;

    quint8 blockReason = 0;

    quint8 ipProto = 0;
    quint16 localPort = 0;
    quint16 remotePort = 0;
    ip_addr_t localIp;
    ip_addr_t remoteIp;

    quint32 pid = 0;

    qint64 connId = 0;
    qint64 appId = 0;

    QString appPath;

    QDateTime connTime;
};

class ConnBlockListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit ConnBlockListModel(QObject *parent = nullptr);

    bool resolveAddress() const { return m_resolveAddress; }
    void setResolveAddress(bool v);

    FortManager *fortManager() const;
    StatBlockManager *statBlockManager() const;
    SqliteDb *sqliteDb() const override;
    AppInfoCache *appInfoCache() const;
    HostInfoCache *hostInfoCache() const;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void deleteConn(qint64 connIdTo);

    const ConnRow &connRowAt(int row) const;

public slots:
    void clear();

protected slots:
    void updateConnIdRange();

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_connRow; }

    void fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const override { }

    int doSqlCount() const override;
    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlLimitOffset() const override;

private:
    QVariant headerDataDisplay(int section, int role) const;

    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDisplayDirection(const ConnRow &connRow, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;

    static QString blockReasonText(const ConnRow &connRow);
    static QString connIconPath(const ConnRow &connRow);

    qint64 connIdMin() const { return m_connIdMin; }
    qint64 connIdMax() const { return m_connIdMax; }

    QString formatIpPort(const ip_addr_t &ip, quint16 port, bool isIPv6) const;

    void updateConnRows(qint64 oldIdMin, qint64 oldIdMax, qint64 idMin, qint64 idMax);
    void resetConnRows(qint64 idMin, qint64 idMax);
    void removeConnRows(qint64 idMin, int count);
    void insertConnRows(qint64 idMax, int endRow, int count);

private:
    uint m_resolveAddress : 1 = false;

    qint64 m_connIdMin = 0;
    qint64 m_connIdMax = 0;

    mutable ConnRow m_connRow;
};

#endif // CONNBLOCKLISTMODEL_H
