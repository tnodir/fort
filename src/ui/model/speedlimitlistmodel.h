#ifndef SPEEDLIMITLISTMODEL_H
#define SPEEDLIMITLISTMODEL_H

#include <sqlite/sqlite_types.h>

#include <conf/speedlimit.h>
#include <util/ioc/iocservice.h>
#include <util/model/tablesqlmodel.h>

enum class SpeedLimitListColumn : qint8 {
    Name = 0,
    Queue,
    Dropped,
    Direction,
    ModTime,
    Count,
};

struct SpeedLimitRow : TableRow, public SpeedLimit
{
};

class SpeedLimitListModel : public TableSqlModel, public IocService
{
    Q_OBJECT

public:
    explicit SpeedLimitListModel(QObject *parent = nullptr);

    SqliteDb *sqliteDb() const override;

    void setUp() override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const SpeedLimitRow &speedLimitRowAt(int row) const;

    static QString menuLabel(const SpeedLimitRow &speedLimitRow);

    void setStatusData(const QByteArray &v);

    static QString columnName(const SpeedLimitListColumn column);

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_speedLimitRow; }

    bool updateSpeedLimitRow(
            const QString &sql, const QVariantHash &vars, SpeedLimitRow &speedLimitRow) const;

    QString sqlBase() const override;

private:
    QVariant headerDataDisplay(int section, int role) const;
    QVariant headerDataDecoration(int section) const;

    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

private:
    QByteArray m_statusData; // FORT_SPEED_LIMITS_STATUS from the driver

    mutable SpeedLimitRow m_speedLimitRow;
};

#endif // SPEEDLIMITLISTMODEL_H
