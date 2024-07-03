#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include <sqlite/sqlite_types.h>

#include <conf/app.h>
#include <util/model/ftstablesqlmodel.h>

class AppGroup;
class AppInfoCache;
class ConfAppManager;
class ConfManager;

struct AppRow : TableRow, public App
{
    const App &app() const { return *this; }
};

class AppListModel : public FtsTableSqlModel
{
    Q_OBJECT

public:
    enum FilterFlag {
        FilterNone = 0,
        FilterWildcard = (1 << 0),
        FilterParked = (1 << 1),
    };
    Q_ENUM(FilterFlag)
    Q_DECLARE_FLAGS(FilterFlags, FilterFlag)

    explicit AppListModel(QObject *parent = nullptr);

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
    AppRow appRowById(qint64 appId) const;
    AppRow appRowByPath(const QString &appPath) const;

signals:
    void filtersChanged();

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_appRow; }

    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlWhereFts() const override;
    QString sqlOrderColumn() const override;

private:
    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataForeground(const QModelIndex &index) const;
    QVariant dataTextAlignment(const QModelIndex &index) const;

    QIcon appIcon(const AppRow &appRow) const;

    bool updateAppRow(const QString &sql, const QVariantHash &vars, AppRow &appRow) const;

private:
    FilterFlags m_filters = FilterNone;
    FilterFlags m_filterValues = FilterNone;

    mutable AppRow m_appRow;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AppListModel::FilterFlags)

#endif // APPLISTMODEL_H
