#ifndef ZONELISTMODEL_H
#define ZONELISTMODEL_H

#include <QDateTime>

#include "../util/model/tablesqlmodel.h"

class ConfManager;
class SqliteDb;
class ZoneSourceWrapper;

struct ZoneRow : TableRow
{
    bool enabled = true;
    bool customUrl = false;

    int zoneId = 0;

    QString zoneName;
    QString sourceCode;

    QString url;
    QString formData;

    QString textChecksum;
    QString binChecksum;

    QDateTime sourceModTime;
    QDateTime lastRun;
    QDateTime lastSuccess;
};

class ZoneListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit ZoneListModel(ConfManager *confManager, QObject *parent = nullptr);

    ConfManager *confManager() const { return m_confManager; }
    SqliteDb *sqliteDb() const override;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    const ZoneRow &zoneRowAt(int row) const;

    bool addZone(const QString &zoneName, const QString &sourceCode, const QString &url,
            const QString &formData, bool enabled, bool customUrl, int &zoneId);
    bool updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
            const QString &url, const QString &formData, bool enabled, bool customUrl,
            bool updateDriver = true);
    bool updateZoneName(int zoneId, const QString &zoneName);
    bool updateZoneEnabled(int zoneId, bool enabled);
    bool updateZoneResult(int zoneId, const QString &textChecksum, const QString &binChecksum,
            const QDateTime &sourceModTime, const QDateTime &lastRun, const QDateTime &lastSuccess);
    void deleteZone(int zoneId, int row);

    QString zoneNameById(int zoneId);

    QVariant zoneTypeByCode(const QString &typeCode) const;

    QVariant zoneSourceByCode(const QString &sourceCode) const;
    const QVariantList &zoneSources() const { return m_zoneSources; }

signals:
    void zoneRemoved(int zoneId);

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_zoneRow; }

    QString sqlBase() const override;

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

    void initZoneTypes();
    void initZoneSources();
    void initZoneSourceNames();

private:
    ConfManager *m_confManager = nullptr;

    mutable ZoneRow m_zoneRow;

    QVariantList m_zoneTypes;
    QVariantHash m_zoneTypesMap;

    QVariantList m_zoneSources;
    QVariantHash m_zoneSourcesMap;
};

#endif // ZONELISTMODEL_H
