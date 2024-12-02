#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include <util/classhelpers.h>

class FirewallConf;

class FortManager : public QObject
{
    Q_OBJECT

public:
    explicit FortManager(QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    bool checkRunningInstance(bool isService, bool isLaunch);

    void initialize();

    static void install(const char *arg);
    static void uninstall(const char *arg = nullptr);

signals:
    void aboutToDestroy();

public slots:
    bool installDriver();
    bool removeDriver();

    void processRestartRequired(const QString &info = {});

    static void setupPortableResource();
    static void setupResources();

private:
    void setupThreadPool();

    void setupLogger();
    void updateLogger();

    void createManagers();
    void deleteManagers();

    bool setupDriver();
    void closeDriver();

    void checkRemoveDriver();

    void checkReinstallDriver();
    void checkStartService();
    void checkDriverAccess();

    void setupEnvManager();
    void setupConfManager();
    void setupConfRuleManager();
    void setupQuotaManager();
    void setupTaskManager();
    void setupServiceInfoManager();

    void loadConf();

    bool setupDriverConf();
    bool updateDriverConf(bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

private:
    bool m_initialized : 1 = false;

    void *m_instanceMutex = nullptr;
};

#endif // FORTMANAGER_H
