#ifndef TRAFLISTMODEL_H
#define TRAFLISTMODEL_H

#include <util/model/tableitemmodel.h>

#include "traflistcolumn.h"
#include "trafunittype.h"

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
    explicit TrafListModel(QObject *parent = nullptr);

    TrafUnitType::TrafUnit unit() const { return m_unitType.unit(); }
    void setUnit(TrafUnitType::TrafUnit v);

    TrafUnitType::TrafType type() const { return m_unitType.type(); }
    void setType(TrafUnitType::TrafType v);

    qint64 appId() const { return m_appId; }
    void setAppId(qint64 appId);

    bool hasApp() const { return appId() > 0; }

    qint32 minTrafTime() const { return m_minTrafTime; }
    qint32 maxTrafTime() const { return m_maxTrafTime; }

    bool isEmpty() const { return minTrafTime() == 0; }

    StatManager *statManager() const;

    void initialize();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const TrafficRow &trafficRowAt(int row) const;

public slots:
    void reset() override;

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_trafRow; }

    void fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const override { }

    qint32 getTrafTime(int row) const;

    static qint32 getTrafCount(TrafUnitType::TrafType type, qint32 minTrafTime, qint32 maxTrafTime);
    static qint32 getMaxTrafTime(TrafUnitType::TrafType type);

private:
    QVariant headerDataDisplay(int section) const;
    QVariant headerDataDecoration(int section) const;

    QVariant dataDisplay(const QModelIndex &index) const;

private:
    TrafUnitType m_unitType;

    qint64 m_appId = -1LL;

    qint32 m_minTrafTime = 0;
    qint32 m_maxTrafTime = 0;
    qint32 m_trafCount = 0;

    mutable TrafficRow m_trafRow;
};

#endif // TRAFLISTMODEL_H
