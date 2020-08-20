#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include "../util/model/tablesqlmodel.h"

QT_FORWARD_DECLARE_CLASS(AppGroup)
QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(LogEntryBlocked)
QT_FORWARD_DECLARE_CLASS(SqliteDb)

struct AppRow : TableRow
{
    bool useGroupPerm = true;
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
    explicit AppListModel(ConfManager *confManager, QObject *parent = nullptr);

    ConfManager *confManager() const { return m_confManager; }
    FirewallConf *conf() const;
    SqliteDb *sqliteDb() const override;

    AppInfoCache *appInfoCache() const { return m_appInfoCache; }
    void setAppInfoCache(AppInfoCache *v);

    void initialize();

    void addLogEntry(const LogEntryBlocked &logEntry);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const AppRow &appRowAt(int row) const;

    bool addApp(const QString &appPath, const QString &appName, const QDateTime &endTime,
            int groupIndex, bool useGroupPerm, bool blocked);
    bool updateApp(qint64 appId, const QString &appPath, const QString &appName,
            const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool blocked,
            bool updateDriver = true);
    bool updateAppName(qint64 appId, const QString &appName);
    void deleteApp(qint64 appId, const QString &appPath, int row);
    void purgeApps();

    const AppGroup *appGroupAt(int index) const;
    QStringList appGroupNames() const;

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_appRow; }

    QString sqlBase() const override;
    QString sqlOrderColumn() const override;

private:
    ConfManager *m_confManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;

    mutable AppRow m_appRow;
};

#endif // APPLISTMODEL_H
