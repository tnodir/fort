#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>

#include <sqlite/sqlitetypes.h>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>
#include <util/service/serviceinfo.h>

class FirewallConf;
class IniOptions;
class IniUser;
class TaskInfo;
class Zone;

class ConfManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ConfManager(const QString &filePath, QObject *parent = nullptr, quint32 openFlags = 0);
    CLASS_DELETE_COPY_MOVE(ConfManager)

    SqliteDb *sqliteDb() const { return m_sqliteDb.data(); }

    FirewallConf *conf() const { return m_conf; }
    FirewallConf *confToEdit() const { return m_confToEdit; }

    IniUser &iniUser() const;
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
    void saveIniUser(bool edited = false, bool onlyFlags = false);

    QVariant toPatchVariant(bool onlyFlags) const;
    bool saveVariant(const QVariant &confVar);

    bool loadTasks(const QList<TaskInfo *> &taskInfos);
    bool saveTasks(const QList<TaskInfo *> &taskInfos);

    virtual bool exportBackup(const QString &path);
    virtual bool importBackup(const QString &path);

    virtual bool addZone(Zone &zone);
    int getFreeZoneId();
    virtual bool deleteZone(int zoneId);
    virtual bool updateZone(const Zone &zone);
    virtual bool updateZoneName(int zoneId, const QString &zoneName);
    virtual bool updateZoneEnabled(int zoneId, bool enabled);
    bool updateZoneResult(const Zone &zone);

    virtual bool checkPassword(const QString &password);

    bool validateDriver();

    void updateServices();
    void updateDriverServices(const QVector<ServiceInfo> &services, int runningServicesCount);

    void updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);

signals:
    void confChanged(bool onlyFlags);
    void iniChanged(const IniOptions &ini);
    void iniUserChanged(const IniUser &ini, bool onlyFlags);

    void zoneAdded();
    void zoneRemoved(int zoneId);
    void zoneUpdated();

protected:
    void setConf(FirewallConf *newConf);
    FirewallConf *createConf();

private:
    void setupDefault(FirewallConf &conf) const;

    bool validateConf(const FirewallConf &newConf);

    bool updateDriverZoneFlag(int zoneId, bool enabled);

    bool loadFromDb(FirewallConf &conf, bool &isNew);
    bool saveToDb(const FirewallConf &conf);

    void loadExtFlags(IniOptions &ini);
    void saveExtFlags(const IniOptions &ini);

    void saveTasksByIni(const IniOptions &ini);

    bool loadTask(TaskInfo *taskInfo);
    bool saveTask(TaskInfo *taskInfo);

    bool beginTransaction();
    bool commitTransaction(bool ok);
    bool checkEndTransaction(bool ok);

private:
    SqliteDbPtr m_sqliteDb;

    FirewallConf *m_conf = nullptr;
    FirewallConf *m_confToEdit = nullptr;

    IniUser *m_iniUserToEdit = nullptr;
};

#endif // CONFMANAGER_H
