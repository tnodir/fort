#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include <util/model/tablesqlmodel.h>

class AppGroup;
class AppInfoCache;
class ConfManager;
class FirewallConf;
class SqliteDb;

struct AppRow : TableRow
{
    bool useGroupPerm = true;
    bool applyChild = false;
    bool blocked = false;
    bool alerted = false;

    int groupIndex = 0;

    qint64 appId = 0;

    QString appPath;
    QString appName;

    QDateTime endTime;
    QDateTime creatTime;
};

class AppListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit AppListModel(QObject *parent = nullptr);

    ConfManager *confManager() const;
    FirewallConf *conf() const;
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

    QString sqlBase() const override;
    QString sqlOrderColumn() const override;

private:
    QVariant headerDataDisplay(int section) const;

    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDisplayState(const AppRow &appRow, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataFont(const QModelIndex &index) const;
    QVariant dataForeground(const QModelIndex &index) const;
    QVariant dataTextAlignment(const QModelIndex &index) const;

    static QString appStateText(const AppRow &appRow);

    QColor appGroupColor(const AppRow &appRow) const;
    static QColor appStateColor(const AppRow &appRow);

    static QIcon appStateIcon(const AppRow &appRow);

    bool updateAppRow(const QString &sql, const QVariantList &vars, AppRow &appRow) const;

private:
    mutable AppRow m_appRow;
};

#endif // APPLISTMODEL_H
