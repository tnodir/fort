#ifndef CONNLISTMODEL_H
#define CONNLISTMODEL_H

#include <common/fortdef.h>
#include <util/conf/conn.h>
#include <util/model/tablesqlmodel.h>

#include "connlistcolumn.h"

// The connection with its ids in the databases
struct ConnRow : TableRow, Conn
{
    quint32 confAppId = 0;

    qint64 connId = 0;
    qint64 appId = 0;
};

class ConnListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit ConnListModel(QObject *parent = nullptr);

    bool resolveAddress() const { return m_resolveAddress; }
    void setResolveAddress(bool v) { m_resolveAddress = v; }

    SqliteDb *sqliteDb() const override;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const ConnRow &connRowAt(int row) const;

    QString rowsAsFilter(const QVector<int> &rows) const;

    static QString reasonText(FortConnReason reason);
    static QString reasonDetailsText(const ConnRow &connRow);
    static QString inheritedText(const ConnRow &connRow);

    static QString directionText(bool inbound);
    static QString directionIconPath(bool inbound);
    static QString actionText(bool blocked, bool dropped = false);

    static QString columnName(const ConnListColumn column);

protected slots:
    virtual void updateConnIdRange();
    virtual void clearConnRows();

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_connRow; }
    ConnRow &connRow() const { return m_connRow; }

    void fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const override { }

    static void fillConnRow(ConnRow &connRow, const SqliteStmt &stmt);

    virtual void fillConnIdRange(qint64 &idMin, qint64 &idMax);

    virtual bool isConnIdRangeOut(
            qint64 oldIdMin, qint64 oldIdMax, qint64 idMin, qint64 idMax) const;

    virtual qint64 connIdByIndex(int row) const;

    int doSqlCount() const override;
    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlLimitOffset() const override;

    void resetConnRows(qint64 idMin, qint64 idMax);

private:
    qint64 connIdMin() const { return m_connIdMin; }
    void setConnIdMin(qint64 v) { m_connIdMin = v; }

    qint64 connIdMax() const { return m_connIdMax; }
    void setConnIdMax(qint64 v) { m_connIdMax = v; }

    int connIdCount() const;

    QVariant headerDataDisplay(int section, int role) const;
    QVariant headerDataDecoration(int section) const;

    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataDecorationAction(const ConnRow &connRow) const;

    void updateConnRows(qint64 oldIdMin, qint64 oldIdMax, qint64 idMin, qint64 idMax);
    void removeConnRows(qint64 idMin, int count);
    void insertConnRows(qint64 idMax, int count);

private:
    uint m_resolveAddress : 1 = false;

    qint64 m_connIdMin = 0;
    qint64 m_connIdMax = 0;

    mutable ConnRow m_connRow;
};

#endif // CONNLISTMODEL_H
