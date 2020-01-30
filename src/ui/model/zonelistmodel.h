#ifndef ZONELISTMODEL_H
#define ZONELISTMODEL_H

#include <QDateTime>

#include "../util/model/tablesqlmodel.h"

QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(SqliteDb)

struct ZoneRow : TableRow {
    bool enabled = true;

    qint64 zoneId = 0;

    QString zoneName;
    QString zoneType;

    QString url;
    QString formData;

    QDateTime lastRun;
    QDateTime lastSuccess;
};

class ZoneListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit ZoneListModel(ConfManager *confManager,
                           QObject *parent = nullptr);

    ConfManager *confManager() const { return m_confManager; }
    SqliteDb *sqliteDb() const override;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const ZoneRow &zoneRowAt(int row) const;

    bool addZone(const QString &zoneName, const QString &zoneType,
                 const QString &url, const QString &formData,
                 bool enabled);
    bool updateZone(qint64 zoneId, const QString &zoneName, const QString &zoneType,
                    const QString &url, const QString &formData,
                    bool enabled, bool updateDriver = true);
    void deleteZone(qint64 zoneId, int row);

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_zoneRow; }

    QString sqlBase() const override;

private:
    ConfManager *m_confManager = nullptr;

    mutable ZoneRow m_zoneRow;
};

#endif // ZONELISTMODEL_H
