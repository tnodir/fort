#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include <sqlite/sqlitetypes.h>

#include <conf/app.h>
#include <util/model/ftstablesqlmodel.h>

class AppGroup;
class AppInfoCache;
class ConfAppManager;
class ConfManager;

struct AppRow : TableRow, public App
{
};

class AppListModel : public FtsTableSqlModel
{
    Q_OBJECT

public:
    explicit AppListModel(QObject *parent = nullptr);

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
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_appRow; }

    QString sqlBase() const override;
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
    mutable AppRow m_appRow;
};

#endif // APPLISTMODEL_H
