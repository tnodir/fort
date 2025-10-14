#ifndef ZONELISTMODEL_H
#define ZONELISTMODEL_H

#include <sqlite/sqlite_types.h>

#include <conf/zone.h>
#include <util/ioc/iocservice.h>
#include <util/model/tablesqlmodel.h>

class ZoneSourceWrapper;

struct ZoneRow : TableRow, public Zone
{
};

class ZoneListModel : public TableSqlModel, public IocService
{
    Q_OBJECT

public:
    explicit ZoneListModel(QObject *parent = nullptr);

    SqliteDb *sqliteDb() const override;

    void setUp() override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const ZoneRow &zoneRowAt(int row) const;

    QVariant zoneTypeById(int typeId) const;
    QVariant zoneTypeByCode(const QString &typeCode) const;
    const QVariantList &zoneTypes() const { return m_zoneTypes; }

    QVariant zoneSourceById(int sourceId) const;
    QVariant zoneSourceByCode(const QString &sourceCode) const;
    const QVariantList &zoneSources() const { return m_zoneSources; }

    static QString zoneSourceTitleById(const int sourceId);

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_zoneRow; }

    bool updateZoneRow(const QString &sql, const QVariantHash &vars, ZoneRow &zoneRow) const;

    QString sqlBase() const override;

private:
    QVariant headerDataDisplay(int section) const;
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

    void setupZoneTypes();
    void setupZoneSources();
    void setupZoneSourceNames();

private:
    QVariantList m_zoneTypes;
    QVariantList m_zoneSources;

    mutable ZoneRow m_zoneRow;
};

#endif // ZONELISTMODEL_H
