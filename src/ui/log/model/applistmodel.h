#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include "../util/model/tableitemmodel.h"

QT_FORWARD_DECLARE_CLASS(AppGroup)
QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(LogEntryBlocked)
QT_FORWARD_DECLARE_CLASS(SqliteDb)

struct AppRow {
    bool isValid(int row) const { return row == this->row; }
    void invalidate() { row = -1; }

    int row = -1;

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

class AppListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit AppListModel(ConfManager *confManager,
                          QObject *parent = nullptr);

    ConfManager *confManager() const { return m_confManager; }
    FirewallConf *conf() const;
    SqliteDb *sqliteDb() const;

    AppInfoCache *appInfoCache() const { return m_appInfoCache; }
    void setAppInfoCache(AppInfoCache *v);

    void initialize();

    void addLogEntry(const LogEntryBlocked &logEntry);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    const AppRow &appRowAt(int row) const;

    bool addApp(const QString &appPath, const QString &appName,
                const QDateTime &endTime, int groupIndex, bool useGroupPerm,
                bool blocked, bool updateDriver = true);
    bool updateApp(qint64 appId, const QString &appPath, const QString &appName,
                   const QDateTime &endTime, int groupIndex, bool useGroupPerm,
                   bool blocked, bool updateDriver = true);
    bool updateAppName(qint64 appId, const QString &appName);
    void deleteApp(qint64 appId, const QString &appPath, int row);

    const AppGroup *appGroupAt(int index) const;
    QStringList appGroupNames() const;

public slots:
    void reset();
    void refresh();

private:
    void invalidateRowCache();
    void updateRowCache(int row) const;

    QString sqlCount() const;
    QString sql() const;
    QString sqlBase() const;
    QString sqlOrder() const;

    bool appBlockedByGroup(const AppRow &appRow) const;
    QString appStateToString(const AppRow &appRow) const;

private:
    int m_sortColumn = 5;
    Qt::SortOrder m_sortOrder = Qt::DescendingOrder;

    mutable int m_appCount = -1;

    ConfManager *m_confManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;

    mutable AppRow m_appRow;
};

#endif // APPLISTMODEL_H
