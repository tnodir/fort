#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include "../util/model/stringlistmodel.h"

class AppInfoCache;
class LogEntryBlockedIp;
class LogEntryProcNew;
class LogEntryStatTraf;
class StatManager;
class TrafListModel;

class AppStatModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppStatModel(StatManager *statManager, QObject *parent = nullptr);

    void initialize();

    TrafListModel *trafListModel() const { return m_trafListModel; }

    AppInfoCache *appInfoCache() const { return m_appInfoCache; }
    void setAppInfoCache(AppInfoCache *v);

    void handleLogProcNew(const LogEntryProcNew &entry, qint64 unixTime);
    void handleLogStatTraf(const LogEntryStatTraf &entry, qint64 unixTime);

    qint64 appIdByRow(int row) const;
    QString appPathByRow(int row) const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void clear() override;

    void remove(int row = -1) override;

private slots:
    void handleCreatedApp(qint64 appId, const QString &appPath);

private:
    void updateList();

private:
    StatManager *m_statManager = nullptr;
    TrafListModel *m_trafListModel = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;

    QVector<qint64> m_appIds;
};

#endif // APPSTATMODEL_H
