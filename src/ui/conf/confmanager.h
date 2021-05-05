#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>
#include <QTimer>

#include "../util/classhelpers.h"
#include "../util/conf/confappswalker.h"

class DriverManager;
class EnvManager;
class FirewallConf;
class FortManager;
class FortSettings;
class SqliteDb;
class SqliteStmt;
class TaskInfo;

class ConfManager : public QObject, public ConfAppsWalker
{
    Q_OBJECT

public:
    explicit ConfManager(const QString &filePath, FortManager *fortManager,
            QObject *parent = nullptr, quint32 openFlags = 0);
    ~ConfManager() override;
    CLASS_DELETE_COPY_MOVE(ConfManager)

    int confVersion() const { return m_confVersion; }
    void setConfVersion(int v) { m_confVersion = v; }

    FortManager *fortManager() const { return m_fortManager; }
    DriverManager *driverManager() const;
    EnvManager *envManager() const;
    FortSettings *settings() const;
    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    FirewallConf *conf() const { return m_conf; }
    FirewallConf *confToEdit() const { return m_confToEdit; }

    bool initialize();

    void initConfToEdit();
    void setConfToEdit(FirewallConf *conf);

    bool load(bool onlyFlags = false);

    virtual bool saveConf(FirewallConf &newConf, bool onlyFlags);
    void applySavedConf(FirewallConf *newConf, bool onlyFlags = false);
    bool save(FirewallConf *newConf, bool onlyFlags = false);
    bool saveFlags();

    bool saveVariant(const QVariant &v, int confVersion, bool onlyFlags = false);

    bool loadTasks(const QList<TaskInfo *> &taskInfos);
    bool saveTasks(const QList<TaskInfo *> &taskInfos);

    qint64 appIdByPath(const QString &appPath);
    bool addApp(const QString &appPath, const QString &appName, const QDateTime &endTime,
            qint64 groupId, bool useGroupPerm, bool blocked, bool alerted = false);
    bool deleteApp(qint64 appId);
    bool updateApp(qint64 appId, const QString &appName, const QDateTime &endTime, qint64 groupId,
            bool useGroupPerm, bool blocked);
    bool updateAppName(qint64 appId, const QString &appName);

    bool walkApps(const std::function<walkAppsCallback> &func) override;

    int appEndsCount();
    void updateAppEndTimes();
    void checkAppEndTimes();

    bool addZone(const QString &zoneName, const QString &sourceCode, const QString &url,
            const QString &formData, bool enabled, bool customUrl, int &zoneId);
    int getFreeZoneId();
    bool deleteZone(int zoneId);
    bool updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
            const QString &url, const QString &formData, bool enabled, bool customUrl);
    bool updateZoneName(int zoneId, const QString &zoneName);
    bool updateZoneEnabled(int zoneId, bool enabled);
    bool updateZoneResult(int zoneId, const QString &textChecksum, const QString &binChecksum,
            const QDateTime &sourceModTime, const QDateTime &lastRun, const QDateTime &lastSuccess);

    bool validateDriver();
    bool updateDriverConf(bool onlyFlags = false);
    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const QString &appPath, int groupIndex, bool useGroupPerm,
            bool blocked, bool remove = false);
    void updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);
    bool updateDriverZoneFlag(int zoneId, bool enabled);

signals:
    void confChanged(bool onlyFlags);
    void appEndTimesUpdated();
    void alertedAppAdded();

protected:
    virtual void setupAppEndTimer();

    void setConf(FirewallConf *newConf);
    FirewallConf *createConf();

    void showErrorMessage(const QString &errorMessage);

private:
    bool checkResult(bool ok, bool commit = false);

    void setupDefault(FirewallConf &conf) const;

    bool loadFromDb(FirewallConf &conf, bool &isNew);
    bool saveToDb(const FirewallConf &conf);

    bool removeAppGroupsInDb(const FirewallConf &conf);

    bool loadTask(TaskInfo *taskInfo);
    bool saveTask(TaskInfo *taskInfo);

private:
    int m_confVersion = 0;

    FortManager *m_fortManager = nullptr;
    SqliteDb *m_sqliteDb = nullptr;

    FirewallConf *m_conf = nullptr;
    FirewallConf *m_confToEdit = nullptr;

    QTimer m_appEndTimer;
};

#endif // CONFMANAGER_H
