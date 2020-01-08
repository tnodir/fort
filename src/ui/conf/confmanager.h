#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>
#include <QTimer>

#include <functional>

#include "../util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(DriverManager)
QT_FORWARD_DECLARE_CLASS(EnvManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_CLASS(SqliteStmt)
QT_FORWARD_DECLARE_CLASS(TaskInfo)

using walkAppsCallback = bool(int groupIndex, bool useGroupPerm,
    bool blocked, bool alerted, const QString &appPath);

class ConfManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ConfManager(const QString &filePath,
                         DriverManager *driverManager,
                         EnvManager *envManager,
                         FortSettings *fortSettings,
                         QObject *parent = nullptr);
    ~ConfManager() override;
    CLASS_DELETE_COPY_MOVE(ConfManager)

    bool initialize();

    FirewallConf *cloneConf(const FirewallConf &conf,
                            QObject *parent = nullptr);

    bool load(FirewallConf &conf);
    bool save(const FirewallConf &conf, bool onlyFlags = false);

    bool loadTasks(const QList<TaskInfo *> &taskInfos);
    bool saveTasks(const QList<TaskInfo *> &taskInfos);

    int appCount(const QString &sql);
    bool getAppByIndex(bool &useGroupPerm, bool &blocked, bool &alerted,
                       qint64 &appId, int &groupIndex, QString &appPath,
                       QDateTime &endTime, QDateTime &creatTime,
                       const QString &sql, const QVariantList &vars);
    qint64 appGroupIdByIndex(int index = 0);
    QStringList appGroupNames();

    bool addApp(const QString &appPath, const QDateTime &endTime,
                int groupIndex, bool useGroupPerm,
                bool blocked, bool alerted);
    bool deleteApp(qint64 appId);
    bool updateApp(qint64 appId, const QDateTime &endTime,
                   int groupIndex, bool useGroupPerm, bool blocked);
    bool walkApps(std::function<walkAppsCallback> func);

    int appEndsCount();
    void updateAppEndTimes();
    void checkAppEndTimes();

    bool updateDriverConf(const FirewallConf &conf, bool onlyFlags = false);
    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const QString &appPath,
                               int groupIndex, bool useGroupPerm,
                               bool blocked, bool alerted);

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();
    void confSaved();
    void appEndTimesUpdated();

private:
    void setErrorMessage(const QString &errorMessage);

    void setupDefault(FirewallConf &conf) const;

    bool loadFromDb(FirewallConf &conf, bool &isNew);
    bool saveToDb(const FirewallConf &conf);

    bool loadTask(TaskInfo *taskInfo);
    bool saveTask(TaskInfo *taskInfo);

private:
    QString m_errorMessage;

    DriverManager *m_driverManager = nullptr;
    EnvManager *m_envManager = nullptr;
    FortSettings *m_fortSettings = nullptr;
    SqliteDb *m_sqliteDb = nullptr;

    QTimer m_appEndTimer;
};

#endif // CONFMANAGER_H
