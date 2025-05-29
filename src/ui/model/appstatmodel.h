#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include <sqlite/sqlite_types.h>

#include <util/model/tablesqlmodel.h>

#include "appstatcolumn.h"
#include "trafunittype.h"

class AppInfoCache;
class StatManager;

struct AppStatRow : TableRow
{
    quint32 confAppId = 0;

    qint64 appId = 0;

    qint64 inBytes = 0;
    qint64 outBytes = 0;

    QString appPath;
};

class AppStatModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit AppStatModel(QObject *parent = nullptr);

    TrafUnitType::TrafUnit unit() const { return m_unitType.unit(); }
    void setUnit(TrafUnitType::TrafUnit v);

    TrafUnitType::TrafType type() const { return m_unitType.type(); }
    void setType(TrafUnitType::TrafType v);

    qint32 trafTime() const { return m_trafTime; }
    void setTrafTime(qint32 v);

    StatManager *statManager() const;
    AppInfoCache *appInfoCache() const;
    SqliteDb *sqliteDb() const override;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const AppStatRow &appStatRowAt(int row) const;

    static QString columnName(const AppStatColumn column);

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_appStatRow; }

    QString sqlBase() const override;
    QString sqlOrderColumn() const override;

private:
    QVariant headerDataDisplay(int section, int role) const;
    QVariant headerDataDecoration(int section) const;

    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataFont(const QModelIndex &index) const;

private:
    TrafUnitType m_unitType;

    qint32 m_trafTime = -1;

    mutable AppStatRow m_appStatRow;
};

#endif // APPSTATMODEL_H
