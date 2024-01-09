#ifndef CONFAPPMANAGER_H
#define CONFAPPMANAGER_H

#include <QObject>

#include <sqlite/sqlitetypes.h>

#include <util/classhelpers.h>
#include <util/conf/confappswalker.h>
#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>

class App;
class ConfManager;
class FirewallConf;
class LogEntryBlocked;

class ConfAppManager : public QObject, public ConfAppsWalker, public IocService
{
    Q_OBJECT

public:
    explicit ConfAppManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfAppManager)

    ConfManager *confManager() const;
    SqliteDb *sqliteDb() const;

    FirewallConf *conf() const;

    void setUp() override;

    void logBlockedApp(const LogEntryBlocked &logEntry);

    qint64 appIdByPath(const QString &appPath);

    virtual bool addApp(const App &app);
    virtual void deleteApps(const QVector<qint64> &appIdList);
    virtual bool purgeApps();
    virtual bool updateApp(const App &app);
    virtual void updateAppsBlocked(
            const QVector<qint64> &appIdList, bool blocked, bool killProcess);
    virtual bool updateAppName(qint64 appId, const QString &appName);

    bool walkApps(const std::function<walkAppsCallback> &func) override;

    bool saveAppBlocked(const App &app);
    void updateAppEndTimes();

    virtual bool updateDriverConf(bool onlyFlags = false);

signals:
    void appAlerted();
    void appChanged();
    void appUpdated();

protected:
    virtual void purgeAppsOnStart();

    virtual void setupAppEndTimer();
    void updateAppEndTimer();

private:
    void setupDriveListManager();

    bool deleteApp(qint64 appId, bool &isWildcard);

    bool updateAppBlocked(qint64 appId, bool blocked, bool killProcess, bool &isWildcard);
    bool prepareAppBlocked(App &app, bool blocked, bool killProcess);

private:
    void emitAppAlerted();
    void emitAppChanged();
    void emitAppUpdated();

    bool addOrUpdateApp(const App &app);

    bool loadAppById(App &app);
    static void fillApp(App &app, const SqliteStmt &stmt);

    bool updateDriverDeleteApp(const QString &appPath);
    bool updateDriverUpdateApp(const App &app, bool remove = false);
    bool updateDriverUpdateAppConf(const App &app);

    bool beginTransaction();
    bool commitTransaction(bool ok);
    bool checkEndTransaction(bool ok);

private:
    quint32 m_driveMask = 0;

    ConfManager *m_confManager = nullptr;

    TriggerTimer m_appAlertedTimer;
    TriggerTimer m_appChangedTimer;
    TriggerTimer m_appUpdatedTimer;

    QTimer m_appEndTimer;
};

#endif // CONFAPPMANAGER_H
