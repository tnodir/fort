#ifndef TIMEPERIODLISTMODEL_H
#define TIMEPERIODLISTMODEL_H

#include <sqlite/sqlite_types.h>

#include <conf/timeperiod.h>
#include <util/ioc/iocservice.h>
#include <util/model/tablesqlmodel.h>

enum class TimePeriodListColumn : qint8 {
    Name = 0,
    Intervals,
    ModTime,
    Count,
};

struct TimePeriodRow : TableRow, public TimePeriod
{
};

class TimePeriodListModel : public TableSqlModel, public IocService
{
    Q_OBJECT

public:
    explicit TimePeriodListModel(QObject *parent = nullptr);

    SqliteDb *sqliteDb() const override;

    void setUp() override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const TimePeriodRow &timePeriodRowAt(int row) const;

    static QString columnName(const TimePeriodListColumn column);

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_timePeriodRow; }

    bool updateTimePeriodRow(
            const QString &sql, const QVariantHash &vars, TimePeriodRow &timePeriodRow) const;

    QString sqlBase() const override;

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

private:
    mutable TimePeriodRow m_timePeriodRow;
};

#endif // TIMEPERIODLISTMODEL_H
