#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include "../util/model/tableitemmodel.h"

QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(LogEntryBlocked)

enum AppState {
    AppAlert = 0,
    AppBlock,
    AppAllow
};

struct AppRow {
    bool isValid(int row) const { return row == this->row; }
    void invalidate() { row = -1; }

    bool blocked() const { return state == AppBlock; }

    int row = -1;

    bool useGroupPerm = true;
    AppState state = AppAlert;
    int groupIndex = 0;

    qint64 appId = 0;

    QString appPath;

    QDateTime endTime;
    QDateTime creatTime;
};

class AppListModel : public TableItemModel
{
    Q_OBJECT

public:
    Q_ENUM(AppState)

    explicit AppListModel(ConfManager *confManager,
                          QObject *parent = nullptr);

    ConfManager *confManager() const { return m_confManager; }

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

    QString appPathByRow(int row) const;
    AppRow appRow(int row) const;

    bool addApp(const QString &appPath, int groupIndex,
                bool useGroupPerm, bool blocked,
                const QDateTime &endTime = QDateTime());
    bool updateApp(qint64 appId, const QString &appPath,
                   int groupIndex, bool useGroupPerm, bool blocked,
                   const QDateTime &endTime = QDateTime());
    void deleteApp(qint64 appId, const QString &appPath, int row);

    QStringList appGroupNames() const { return m_appGroupNames; }
    QString appGroupNameByIndex(int groupIndex) const;

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

    void updateAppGroupNames();

    QString appStateToString(AppState state) const;

private:
    int m_sortColumn = 5;
    Qt::SortOrder m_sortOrder = Qt::DescendingOrder;

    mutable int m_appCount = -1;

    QStringList m_appGroupNames;

    ConfManager *m_confManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;

    mutable AppRow m_rowCache;
};

#endif // APPLISTMODEL_H
