#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>
#include <QTimer>

#include "../util/classhelpers.h"
#include "../util/conf/confappswalker.h"

QT_FORWARD_DECLARE_CLASS(DriverManager)
QT_FORWARD_DECLARE_CLASS(EnvManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_CLASS(SqliteStmt)
QT_FORWARD_DECLARE_CLASS(TaskInfo)

class ConfManager : public QObject, public ConfAppsWalker
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ConfManager(const QString &filePath,
                         FortManager *fortManager,
                         QObject *parent = nullptr);
    ~ConfManager() override;
    CLASS_DELETE_COPY_MOVE(ConfManager)

    FortManager *fortManager() const { return m_fortManager; }
    DriverManager *driverManager() const;
    EnvManager *envManager() const;
    FortSettings *settings() const;
    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    bool isEditing() const { return confToEdit() != nullptr; }

    FirewallConf *conf() const { return m_conf; }
    FirewallConf *confToEdit() const { return m_confToEdit; }

    bool initialize();

    void setConfToEdit(FirewallConf *conf);
    FirewallConf *cloneConf(const FirewallConf &conf,
                            QObject *parent = nullptr) const;

    bool load(FirewallConf &conf);
    bool save(FirewallConf &newConf, bool onlyFlags = false);

    bool loadTasks(const QList<TaskInfo *> &taskInfos);
    bool saveTasks(const QList<TaskInfo *> &taskInfos);

    bool addApp(const QString &appPath, const QString &appName,
                const QDateTime &endTime,
                qint64 groupId, bool useGroupPerm,
                bool blocked, bool alerted = false);
    bool deleteApp(qint64 appId);
    bool updateApp(qint64 appId, const QString &appName, const QDateTime &endTime,
                   qint64 groupId, bool useGroupPerm, bool blocked);
    bool updateAppName(qint64 appId, const QString &appName);

    bool walkApps(std::function<walkAppsCallback> func) override;

    int appEndsCount();
    void updateAppEndTimes();
    void checkAppEndTimes();

    bool updateDriverConf(bool onlyFlags = false);
    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const QString &appPath,
                               int groupIndex, bool useGroupPerm,
                               bool blocked, bool alerted = false);

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();
    void isEditingChanged();
    void confSaved(bool onlyFlags);
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

    FortManager *m_fortManager = nullptr;
    SqliteDb *m_sqliteDb = nullptr;

    FirewallConf *m_conf = nullptr;
    FirewallConf *m_confToEdit = nullptr;

    QTimer m_appEndTimer;
};

#endif // CONFMANAGER_H
