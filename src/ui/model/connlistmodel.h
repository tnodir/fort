#ifndef CONNLISTMODEL_H
#define CONNLISTMODEL_H

#include <QDateTime>

#include <common/common_types.h>
#include <common/fortdef.h>
#include <util/model/tablesqlmodel.h>

#include "connlistcolumn.h"

class AppInfoCache;
class FortManager;
class HostInfoCache;
class StatConnManager;

struct ConnRow : TableRow
{
    bool isIPv6 : 1 = false;
    bool blocked : 1 = false;
    bool inherited : 1 = false;
    bool inbound : 1 = false;

    quint8 reason = 0;

    quint8 ipProto = 0;
    quint8 zoneId = 0;
    quint16 ruleId = 0;
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

class ConnListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit ConnListModel(QObject *parent = nullptr);

    bool resolveAddress() const { return m_resolveAddress; }
    void setResolveAddress(bool v);

    FortManager *fortManager() const;
    StatConnManager *statConnManager() const;
    SqliteDb *sqliteDb() const override;
    AppInfoCache *appInfoCache() const;
    HostInfoCache *hostInfoCache() const;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const ConnRow &connRowAt(int row) const;

    QString rowsAsFilter(const QVector<int> &rows) const;

    static QString reasonText(FortConnReason reason);

    static QString columnName(const ConnListColumn column);

protected slots:
    void updateConnIdRange();

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_connRow; }

    void fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const override { }

    qint64 connIdByIndex(int row) const;

    int doSqlCount() const override;
    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlLimitOffset() const override;

private:
    QVariant headerDataDisplay(int section, int role) const;
    QVariant headerDataDecoration(int section) const;

    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;

    qint64 connIdMin() const { return m_connIdMin; }
    qint64 connIdMax() const { return m_connIdMax; }

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

#endif // CONNLISTMODEL_H
