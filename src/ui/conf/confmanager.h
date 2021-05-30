#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>
#include <QTimer>

#include "../util/classhelpers.h"
#include "../util/conf/confappswalker.h"
#include "../util/triggertimer.h"

class AppInfoCache;
class DriverManager;
class EnvManager;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class IniUser;
class LogEntryBlocked;
class SqliteDb;
class SqliteStmt;
class TaskInfo;
class TaskManager;

class ConfManager : public QObject, public ConfAppsWalker
{
    Q_OBJECT

public:
    explicit ConfManager(const QString &filePath, FortManager *fortManager,
            QObject *parent = nullptr, quint32 openFlags = 0);
    ~ConfManager() override;
    CLASS_DELETE_COPY_MOVE(ConfManager)

    FortManager *fortManager() const { return m_fortManager; }
    DriverManager *driverManager() const;
    EnvManager *envManager() const;
    FortSettings *settings() const;
    TaskManager *taskManager() const;
    AppInfoCache *appInfoCache() const;

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    FirewallConf *conf() const { return m_conf; }
    FirewallConf *confToEdit() const { return m_confToEdit; }

    IniUser *iniUser() const;

    bool initialize();

    void initConfToEdit();
    void setConfToEdit(FirewallConf *conf);

    bool loadConf(FirewallConf &conf);
    bool load();

    virtual bool saveConf(FirewallConf &conf);
    void applySavedConf(FirewallConf *newConf);
    bool save(FirewallConf *newConf);

    bool saveFlags();
    void saveIni();
    void saveIniUser(bool flagsChanged = false);

    QVariant toPatchVariant(bool onlyFlags) const;
    bool saveVariant(const QVariant &confVar);

    bool loadTasks(const QList<TaskInfo *> &taskInfos);
    bool saveTasks(const QList<TaskInfo *> &taskInfos);

    void logBlockedApp(const LogEntryBlocked &logEntry);

    qint64 appIdByPath(const QString &appPath);
    virtual bool addApp(const QString &appPath, const QString &appName, const QDateTime &endTime,
            int groupIndex, bool useGroupPerm, bool blocked, bool alerted = false);
    virtual bool deleteApp(qint64 appId, const QString &appPath);
    virtual bool updateApp(qint64 appId, const QString &appPath, const QString &appName,
            const QDateTime &endTime, qint64 groupId, int groupIndex, bool useGroupPerm,
            bool blocked);
    virtual bool updateAppName(qint64 appId, const QString &appName);

    bool walkApps(const std::function<walkAppsCallback> &func) override;

    int appEndsCount();
    void updateAppEndTimes();
    void checkAppEndTimes();

    virtual bool addZone(const QString &zoneName, const QString &sourceCode, const QString &url,
            const QString &formData, bool enabled, bool customUrl, int &zoneId);
    int getFreeZoneId();
    virtual bool deleteZone(int zoneId);
    virtual bool updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
            const QString &url, const QString &formData, bool enabled, bool customUrl);
    virtual bool updateZoneName(int zoneId, const QString &zoneName);
    virtual bool updateZoneEnabled(int zoneId, bool enabled);
    bool updateZoneResult(int zoneId, int addressCount, const QString &textChecksum,
            const QString &binChecksum, const QDateTime &sourceModTime, const QDateTime &lastRun,
            const QDateTime &lastSuccess);

    virtual bool checkPassword(const QString &password);

    bool validateDriver();
    virtual bool updateDriverConf(bool onlyFlags = false);
    void updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);

signals:
    void confChanged(bool onlyFlags);
    void iniUserChanged(bool onlyFlags);

    void appAlerted();
    void appChanged();
    void appUpdated();

    void zoneAdded();
    void zoneRemoved(int zoneId);
    void zoneUpdated();

protected:
    virtual void setupAppEndTimer();

    void setConf(FirewallConf *newConf);
    FirewallConf *createConf();

    void showErrorMessage(const QString &errorMessage);

private:
    void setupDefault(FirewallConf &conf) const;

    void emitAppAlerted();
    void emitAppChanged();
    void emitAppUpdated();

    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const QString &appPath, int groupIndex, bool useGroupPerm,
            bool blocked, bool remove = false);
    bool updateDriverZoneFlag(int zoneId, bool enabled);

    bool loadFromDb(FirewallConf &conf, bool &isNew);
    bool saveToDb(const FirewallConf &conf);

    void loadExtFlags(IniOptions &ini);
    void saveExtFlags(const IniOptions &ini);

    void saveTasksByIni(const IniOptions &ini);

    bool loadTask(TaskInfo *taskInfo);
    bool saveTask(TaskInfo *taskInfo);

    bool checkResult(bool ok, bool commit = false);

private:
    FortManager *m_fortManager = nullptr;
    SqliteDb *m_sqliteDb = nullptr;

    FirewallConf *m_conf = nullptr;
    FirewallConf *m_confToEdit = nullptr;

    TriggerTimer m_appAlertedTimer;
    TriggerTimer m_appChangedTimer;
    TriggerTimer m_appUpdatedTimer;

    QTimer m_appEndTimer;
};

#endif // CONFMANAGER_H
