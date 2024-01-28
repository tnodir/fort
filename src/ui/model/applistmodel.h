#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include <sqlite/sqlitetypes.h>

#include <conf/app.h>
#include <util/model/tablesqlmodel.h>

class AppGroup;
class AppInfoCache;
class ConfAppManager;
class ConfManager;

struct AppRow : TableRow, public App
{
};

class AppListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit AppListModel(QObject *parent = nullptr);

    QString ftsFilterMatch() const { return m_ftsFilterMatch; }
    QString ftsFilter() const { return m_ftsFilter; }
    void setFtsFilter(const QString &filter);

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

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_appRow; }

    void fillSqlVars(QVariantList &vars) const override;

    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlOrderColumn() const override;

private:
    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataForeground(const QModelIndex &index) const;
    QVariant dataTextAlignment(const QModelIndex &index) const;

    QIcon appIcon(const AppRow &appRow) const;

    bool updateAppRow(const QString &sql, const QVariantList &vars, AppRow &appRow) const;

private:
    QString m_ftsFilter;
    QString m_ftsFilterMatch;

    mutable AppRow m_appRow;
};

#endif // APPLISTMODEL_H
