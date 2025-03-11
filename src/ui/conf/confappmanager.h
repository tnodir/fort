#ifndef CONFAPPMANAGER_H
#define CONFAPPMANAGER_H

#include <QObject>

#include <util/classhelpers.h>
#include <util/conf/confappswalker.h>
#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>

#include "confmanagerbase.h"

class App;
class AppGroup;
class ConfManager;
class FirewallConf;
class LogEntryApp;

class ConfAppManager : public ConfManagerBase, public ConfAppsWalker, public IocService
{
    Q_OBJECT

public:
    explicit ConfAppManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfAppManager)

    void setUp() override;

    void logApp(const LogEntryApp &logEntry);

    qint64 appIdByPath(const QString &appOriginPath, QString &normPath);

    virtual bool addOrUpdateAppPath(const QString &appOriginPath, bool blocked, bool killProcess);
    virtual bool deleteAppPath(const QString &appOriginPath);

    virtual bool addOrUpdateApp(App &app, bool onlyUpdate = false);
    virtual bool updateApp(App &app);
    virtual bool updateAppName(qint64 appId, const QString &appName);
    virtual bool deleteApps(const QVector<qint64> &appIdList);
    virtual bool deleteAlertedApps();
    virtual bool clearAlerts();
    virtual bool purgeApps();
    virtual bool updateAppsBlocked(
            const QVector<qint64> &appIdList, bool blocked, bool killProcess);

    bool walkApps(const std::function<walkAppsCallback> &func) const override;

    bool saveAppBlocked(const App &app);
    void updateAppEndTimes();

    qint64 getAlertAppId();

    virtual bool importAppsBackup(const QString &path);

    virtual bool updateDriverConf(bool onlyFlags = false);

signals:
    void appAlerted(bool alerted = true);
    void appsChanged();
    void appUpdated();

protected:
    virtual void setupConfManager();
    virtual void setupDriveListManager();

    virtual void setupAppEndTimer();
    void updateAppEndTimer();

private:
    bool addApp(App &app);

    void beginAddOrUpdateApp(App &app, const AppGroup &appGroup, bool onlyUpdate, bool &ok);
    void endAddOrUpdateApp(const App &app, bool onlyUpdate);

    bool deleteApp(qint64 appId, bool &isWildcard);

    bool updateAppBlocked(qint64 appId, bool blocked, bool killProcess, bool &isWildcard);
    bool checkAppBlockedChanged(App &app, bool blocked, bool killProcess);

    QVector<qint64> collectObsoleteApps(quint32 driveMask);

private:
    void emitAppAlerted();
    void emitAppsChanged();
    void emitAppUpdated();

    bool loadAppById(App &app);
    static void fillApp(App &app, const SqliteStmt &stmt);

    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const App &app, bool remove = false);
    bool updateDriverUpdateAppConf(const App &app);

private:
    quint32 m_driveMask = 0;

    TriggerTimer m_appAlertedTimer;
    TriggerTimer m_appsChangedTimer;
    TriggerTimer m_appUpdatedTimer;

    QTimer m_appEndTimer;
};

#endif // CONFAPPMANAGER_H
