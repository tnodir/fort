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

    App appById(qint64 appId);
    App appByPath(const QString &appPath);

    qint64 appIdByPath(const QString &appOriginPath, QString &normPath);

    virtual bool addOrUpdateAppPath(const QString &appOriginPath, bool blocked, bool killProcess);
    virtual bool deleteAppPath(const QString &appOriginPath);

    virtual bool addOrUpdateApp(App &app, bool onlyUpdate = false);
    virtual bool updateApp(App &app);
    virtual bool updateAppName(qint64 appId, const QString &appName);
    virtual bool deleteApps(const QVector<qint64> &appIdList);
    virtual bool addAlertedApp(qint64 appId);
    virtual bool deleteAlertedApps();
    virtual bool clearAlerts();
    virtual bool purgeApps();
    virtual bool updateAppsBlocked(
            const QVector<qint64> &appIdList, bool blocked, bool killProcess);
    virtual bool updateAppsTimer(const QVector<qint64> &appIdList, int minutes);

    bool walkApps(const std::function<walkAppsCallback> &func) const override;

    qint64 getAlertAppId();

    virtual bool importAppsBackup(const QString &path);

    virtual bool canUpdateDriverConf() const { return true; }
    virtual bool updateDriverConf(bool onlyFlags = false);

signals:
    void appAlerted(bool alerted = true);
    void appsChanged();
    void appUpdated();
    void appDeleted(qint64 appId);

protected:
    virtual void setupConfManager();

    virtual void setupAppEndTimer();
    void updateAppEndTimer();

    void checkAppAlerted();

private:
    bool addApp(App &app);

    void beginAddOrUpdateApp(App &app, const AppGroup &appGroup, bool onlyUpdate, bool &ok);
    void endAddOrUpdateApp(const App &app, bool onlyUpdate);

    bool deleteApp(qint64 appId, bool &isWildcard);

    bool updateAppBlocked(qint64 appId, bool blocked, bool killProcess, bool &isWildcard);
    bool checkAppBlockedChanged(App &app, bool blocked, bool killProcess);

    bool updateAppTimer(qint64 appId, QDateTime scheduleTime, bool &isWildcard);

    QVector<qint64> collectObsoleteApps(quint32 driveMask);

    bool saveAppBlocked(const App &app);
    bool saveAppTimer(const App &app);
    void updateAppEndTimes();

private:
    void emitAppAlerted();
    void emitAppsChanged();
    void emitAppUpdated();

    bool loadAppById(App &app, qint64 appId);
    static void fillApp(App &app, const SqliteStmt &stmt);

    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const App &app, bool remove = false);
    bool updateDriverUpdateAppConf(const App &app);

private:
    TriggerTimer m_appAlertedTimer;
    TriggerTimer m_appsChangedTimer;
    TriggerTimer m_appUpdatedTimer;

    QTimer m_appEndTimer;
};

#endif // CONFAPPMANAGER_H
