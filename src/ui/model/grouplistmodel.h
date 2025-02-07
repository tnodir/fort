#ifndef GROUPLISTMODEL_H
#define GROUPLISTMODEL_H

#include <sqlite/sqlite_types.h>

#include <conf/appgroup.h>
#include <util/ioc/iocservice.h>
#include <util/model/tablesqlmodel.h>

class ConfGroupManager;

struct GroupRow : TableRow, public AppGroup
{
};

class GroupListModel : public TableSqlModel, public IocService
{
    Q_OBJECT

public:
    explicit GroupListModel(QObject *parent = nullptr);

    ConfGroupManager *confGroupManager() const;
    SqliteDb *sqliteDb() const override;

    void setUp() override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const GroupRow &groupRowAt(int row) const;

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_groupRow; }

    bool updateGroupRow(const QString &sql, const QVariantHash &vars, GroupRow &groupRow) const;

    QString sqlBase() const override;

private:
    QVariant headerDataDisplay(int section) const;
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

private:
    mutable GroupRow m_groupRow;
};

#endif // GROUPLISTMODEL_H
