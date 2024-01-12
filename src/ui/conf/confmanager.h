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

    bool exportBackup(const QString &path);
    virtual bool exportMasterBackup(const QString &path);

    bool importBackup(const QString &path);
    virtual bool importMasterBackup(const QString &path);

    virtual bool checkPassword(const QString &password);

    bool validateDriver();

    void updateServices();
    void updateDriverServices(const QVector<ServiceInfo> &services, int runningServicesCount);

signals:
    void confChanged(bool onlyFlags);
    void iniChanged(const IniOptions &ini);
    void iniUserChanged(const IniUser &ini, bool onlyFlags);

protected:
    void setConf(FirewallConf *newConf);
    FirewallConf *createConf();

private:
    bool setupDb();

    void setupDefault(FirewallConf &conf) const;

    bool validateConf(const FirewallConf &newConf);

    bool loadFromDb(FirewallConf &conf, bool &isNew);
    bool saveToDb(const FirewallConf &conf);

    void saveTasksByIni(const IniOptions &ini);

    bool loadTask(TaskInfo *taskInfo);
    bool saveTask(TaskInfo *taskInfo);

    bool beginTransaction();
    void commitTransaction(bool &ok);

private:
    SqliteDbPtr m_sqliteDb;

    FirewallConf *m_conf = nullptr;
    FirewallConf *m_confToEdit = nullptr;

    IniUser *m_iniUserToEdit = nullptr;
};

#endif // CONFMANAGER_H
