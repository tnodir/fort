#ifndef TRAFLISTMODEL_H
#define TRAFLISTMODEL_H

#include "../util/model/tableitemmodel.h"

class StatManager;

struct TrafficRow : TableRow
{
    qint32 trafTime = 0;
    qint64 inBytes = 0;
    qint64 outBytes = 0;
};

class TrafListModel : public TableItemModel
{
    Q_OBJECT

public:
    enum TrafUnit { UnitAdaptive = 0, UnitBytes, UnitKB, UnitMB, UnitGB, UnitTB };
    Q_ENUM(TrafUnit)

    enum TrafType { TrafHourly = 0, TrafDaily, TrafMonthly, TrafTotal };
    Q_ENUM(TrafType)

    explicit TrafListModel(StatManager *statManager, QObject *parent = nullptr);

    TrafListModel::TrafUnit unit() const { return m_unit; }
    void setUnit(TrafListModel::TrafUnit unit);

    TrafListModel::TrafType type() const { return m_type; }
    void setType(TrafListModel::TrafType type);

    qint64 appId() const { return m_appId; }
    void setAppId(qint64 appId);

    StatManager *statManager() const { return m_statManager; }

    void initialize();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void clear();

    void resetAppTotals();

    void resetTraf();
    void reset();

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_trafRow; }

    QString formatTrafUnit(qint64 bytes) const;
    QString formatTrafTime(qint32 trafTime) const;

    qint32 getTrafTime(int row) const;

    static qint32 getTrafCount(TrafType type, qint32 minTrafTime, qint32 maxTrafTime);
    static qint32 getMaxTrafTime(TrafType type);

    static const char *getSqlMinTrafTime(TrafType type, qint64 appId);
    static const char *getSqlSelectTraffic(TrafType type, qint64 appId);

private:
    bool m_isEmpty = false;

    TrafUnit m_unit = UnitAdaptive;
    TrafType m_type = TrafHourly;

    qint64 m_appId = 0;

    qint32 m_minTrafTime = 0;
    qint32 m_maxTrafTime = 0;
    qint32 m_trafCount = 0;

    StatManager *m_statManager = nullptr;

    mutable TrafficRow m_trafRow;
};

#endif // TRAFLISTMODEL_H
