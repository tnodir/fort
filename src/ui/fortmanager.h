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

    bool checkRunningInstance(bool isService);

    void initialize();

    static void install(const char *arg);
    static void uninstall();

public slots:
    bool installDriver();
    bool removeDriver();

    void show();

    void processRestartRequired();

    static void setupPortableResource();
    static void setupResources();

private:
    void setupThreadPool();

    void setupLogger();
    void updateLogger(const FirewallConf *conf);

    void createManagers();
    void deleteManagers();

    bool setupDriver();
    void closeDriver();

    void setupPortableDriver();

    void setupEnvManager();
    void setupConfManager();
    void setupQuotaManager();
    void setupTaskManager();
    void setupServiceInfoManager();

    void loadConf();

    bool updateDriverConf(bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

private:
    bool m_initialized : 1;

    void *m_instanceMutex = nullptr;
};

#endif // FORTMANAGER_H
