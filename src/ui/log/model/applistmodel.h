#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QDateTime>

#include "../util/model/tableitemmodel.h"

QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(LogEntryBlocked)

enum AppState {
    Alert = 0,
    Block,
    Allow
};

struct AppRow {
    bool isValid(int row) const { return row == this->row; }
    void invalidate() { row = -1; }

    int row = -1;

    AppState state = Alert;

    qint64 appId = 0;
    qint64 appGroupId = 0;

    QString appGroupName;
    QString appPath;

    QDateTime endTime;
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

    void addLogEntry(const LogEntryBlocked &logEntry);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QString appPathByRow(int row) const;

public slots:
    void reset();

private:
    void invalidateRowCache();
    void updateRowCache(int row) const;

    QString appStateToString(AppState state) const;

private:
    mutable int m_appCount = -1;

    ConfManager *m_confManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;

    mutable AppRow m_rowCache;
};

#endif // APPLISTMODEL_H
