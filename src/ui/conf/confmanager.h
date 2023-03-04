#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>
#include <QTimer>

#include <sqlite/sqlitetypes.h>

#include <util/classhelpers.h>
#include <util/conf/confappswalker.h>
#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>

class App;
class FirewallConf;
class IniOptions;
class IniUser;
class LogEntryBlocked;
class TaskInfo;
class Zone;

class ConfManager : public QObject, public ConfAppsWalker, public IocService
{
    Q_OBJECT

public:
    explicit ConfManager(const QString &filePath, QObject *parent = nullptr, quint32 openFlags = 0);
    CLASS_DELETE_COPY_MOVE(ConfManager)

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    FirewallConf *conf() const { return m_conf; }
    FirewallConf *confToEdit() const { return m_confToEdit; }

    IniUser *iniUser() const;
    IniUser *iniUserToEdit() const { return m_iniUserToEdit; }

    void setUp() override;

    void initConfToEdit();
    void setConfToEdit(FirewallConf *conf);

    void initIniUserToEdit();
    void setIniUserToEdit(IniUser *iniUser);

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
    virtual bool addApp(const App &app);
    virtual bool deleteApp(qint64 appId);
    virtual bool purgeApps();
    virtual bool updateApp(const App &app);
    virtual bool updateAppBlocked(qint64 appId, bool blocked);
    virtual bool updateAppName(qint64 appId, const QString &appName);

    bool walkApps(const std::function<walkAppsCallback> &func) override;

    void updateAppEndTimes();

    virtual bool addZone(Zone &zone);
    int getFreeZoneId();
    virtual bool deleteZone(int zoneId);
    virtual bool updateZone(const Zone &zone);
    virtual bool updateZoneName(int zoneId, const QString &zoneName);
    virtual bool updateZoneEnabled(int zoneId, bool enabled);
    bool updateZoneResult(const Zone &zone);

    virtual bool checkPassword(const QString &password);

    bool validateDriver();
    virtual bool updateDriverConf(bool onlyFlags = false);
    void updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);

signals:
    void confChanged(bool onlyFlags);
    void iniChanged(const IniOptions &ini);
    void iniUserChanged(bool onlyFlags);

    void appAlerted();
    void appChanged();
    void appUpdated();

    void zoneAdded();
    void zoneRemoved(int zoneId);
    void zoneUpdated();

protected:
    virtual void setupAppEndTimer();
    void updateAppEndTimer();

    void setConf(FirewallConf *newConf);
    FirewallConf *createConf();

    void showErrorMessage(const QString &errorMessage);

private:
    void setupDefault(FirewallConf &conf) const;

    void emitAppAlerted();
    void emitAppChanged();
    void emitAppUpdated();

    bool addOrUpdateApp(const App &app);
    bool updateDriverAppBlocked(qint64 appId, bool blocked, bool &changed);

    bool validateConf(const FirewallConf &newConf);

    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const App &app, bool remove = false);
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
    SqliteDb *m_sqliteDb = nullptr;

    FirewallConf *m_conf = nullptr;
    FirewallConf *m_confToEdit = nullptr;

    IniUser *m_iniUserToEdit = nullptr;

    TriggerTimer m_appAlertedTimer;
    TriggerTimer m_appChangedTimer;
    TriggerTimer m_appUpdatedTimer;

    QTimer m_appEndTimer;
};

#endif // CONFMANAGER_H
