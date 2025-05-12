#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include <sqlite/sqlite_types.h>

#include <conf/app.h>
#include <util/model/ftstablesqlmodel.h>

#include "applistcolumn.h"

class AppGroup;
class AppInfoCache;
class AppListModelData;
class ConfAppManager;
class ConfManager;

struct AppRow : TableRow, public App
{
    const App &app() const { return *this; }
};

struct AppStatesCount
{
    int allowed = 0;
    int blocked = 0;
    int alerted = 0;
};

class AppListModel : public FtsTableSqlModel
{
    Q_OBJECT

public:
    enum SortState : qint8 {
        SortNone = 0,
        SortAllowed,
        SortBlocked,
        SortAlerted,
    };
    Q_ENUM(SortState)

    enum FilterFlag {
        FilterNone = 0,
        FilterAlerted = (1 << 0),
        FilterWildcard = (1 << 1),
        FilterParked = (1 << 2),
        FilterKillProcess = (1 << 3),
    };
    Q_ENUM(FilterFlag)
    Q_DECLARE_FLAGS(FilterFlags, FilterFlag)

    explicit AppListModel(QObject *parent = nullptr);

    SortState sortState() const { return m_sortState; }
    void setSortState(SortState v);

    FilterFlags filters() const { return m_filters; }
    void setFilters(FilterFlags v);
    void setFilter(FilterFlag v, bool on = true);

    FilterFlags filterValues() const { return m_filterValues; }
    void setFilterValue(FilterFlag v, Qt::CheckState checkState);

    void clearFilters();

    ConfManager *confManager() const;
    ConfAppManager *confAppManager() const;
    AppInfoCache *appInfoCache() const;
    SqliteDb *sqliteDb() const override;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const AppRow &appRowAt(int row) const;

    AppStatesCount appStatesCount() const;

    static QString columnName(const AppListColumn column);

signals:
    void sortStateChanged();
    void filtersChanged();

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_appRow; }

    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlWhereFts() const override;
    QString sqlOrderColumn() const override;

    void addSqlFilter(QStringList &list, const QString &name, FilterFlag flag) const;

private:
    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataForeground(const QModelIndex &index) const;
    QVariant dataTextAlignment(const QModelIndex &index) const;

    AppListModelData appDataAt(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    SortState m_sortState = SortNone;

    FilterFlags m_filters = FilterNone;
    FilterFlags m_filterValues = FilterNone;

    mutable AppRow m_appRow;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AppListModel::FilterFlags)

#endif // APPLISTMODEL_H
