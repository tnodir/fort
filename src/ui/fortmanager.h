#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "util/classhelpers.h"

class AppInfoCache;
class AppInfoManager;
class AppListModel;
class ConfManager;
class ControlManager;
class DriverManager;
class EnvManager;
class FirewallConf;
class FortSettings;
class GraphWindow;
class HostInfoCache;
class HotKeyManager;
class IniUser;
class LogManager;
class MainWindow;
class NativeEventFilter;
class OptionsWindow;
class ProgramsWindow;
class QuotaManager;
class RpcManager;
class StatManager;
class StatisticsWindow;
class TaskManager;
class TrayIcon;
class UserSettings;
class ZoneListModel;
class ZonesWindow;

class FortManager : public QObject
{
    Q_OBJECT

public:
    enum TrayMessageType { MessageOptions, MessageZones };
    Q_ENUM(TrayMessageType)

    explicit FortManager(QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    MainWindow *mainWindow() const { return m_mainWindow; }
    HotKeyManager *hotKeyManager() const { return m_hotKeyManager; }

    ProgramsWindow *progWindow() const { return m_progWindow; }
    OptionsWindow *optWindow() const { return m_optWindow; }
    StatisticsWindow *connWindow() const { return m_statWindow; }
    ZonesWindow *zoneWindow() const { return m_zoneWindow; }
    GraphWindow *graphWindow() const { return m_graphWindow; }

    FortSettings *settings() const;
    EnvManager *envManager() const;
    ControlManager *controlManager() const;

    UserSettings *userSettings() const { return m_userSettings; }
    IniUser *iniUser() const;
    RpcManager *rpcManager() const { return m_rpcManager; }

    ConfManager *confManager() const { return m_confManager; }
    FirewallConf *conf() const;
    QuotaManager *quotaManager() const { return m_quotaManager; }
    StatManager *statManager() const { return m_statManager; }
    AppInfoManager *appInfoManager() const { return m_appInfoManager; }
    DriverManager *driverManager() const { return m_driverManager; }
    LogManager *logManager() const { return m_logManager; }
    TaskManager *taskManager() const { return m_taskManager; }

    AppInfoCache *appInfoCache() const { return m_appInfoCache; }
    HostInfoCache *hostInfoCache() const { return m_hostInfoCache; }

    AppListModel *appListModel() const { return m_appListModel; }
    ZoneListModel *zoneListModel() const { return m_zoneListModel; }

    bool checkRunningInstance();

    void initialize();

signals:
    void optWindowChanged(bool visible);
    void graphWindowChanged(bool visible);

public slots:
    bool installDriver();
    bool removeDriver();

    void show();

    void showTrayIcon();
    void closeTrayIcon();
    void showTrayMessage(
            const QString &message, FortManager::TrayMessageType type = MessageOptions);

    void showProgramsWindow();
    void closeProgramsWindow();

    bool showProgramEditForm(const QString &appPath);

    void showOptionsWindow();
    void closeOptionsWindow();
    void reloadOptionsWindow(const QString &reason);

    void showStatisticsWindow();
    void closeStatisticsWindow();

    void showZonesWindow();
    void closeZonesWindow();

    void showGraphWindow();
    void closeGraphWindow(bool wasVisible = false);
    void switchGraphWindow();

    void processRestartRequired();

    void quitByCheckPassword();

    bool checkPassword();

    void showErrorBox(const QString &text, const QString &title = QString());
    void showInfoBox(const QString &text, const QString &title = QString());
    bool showQuestionBox(const QString &text, const QString &title = QString());
    bool showYesNoBox(const QString &text, const QString &yesText, const QString &noText,
            const QString &title = QString());

    static void setupResources();

private:
    void setupThreadPool();

    void setupLogger();
    void updateLogger();

    void createManagers();

    void setupControlManager();
    void setupRpcManager();

    void setupLogManager();
    void closeLogManager();

    void setupDriverManager();
    bool setupDriver();
    void closeDriver();

    void setupEventFilter();
    void setupEnvManager();

    void setupConfManager();
    void setupQuotaManager();
    void setupStatManager();
    void setupAppInfoManager();

    void setupAppInfoCache();
    void setupHostInfoCache();
    void setupModels();

    void setupTaskManager();

    void setupUserSettings();
    void setupTranslationManager();

    void setupMainWindow();
    void closeMainWindow();

    void setupHotKeyManager();
    void setupTrayIcon();

    void setupProgramsWindow();
    void setupOptionsWindow();
    void setupZonesWindow();
    void setupGraphWindow();
    void setupStatisticsWindow();

    void closeUi();

    void loadConf();

    bool updateDriverConf(bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

    void onTrayMessageClicked();

    QWidget *focusWidget() const;
    static void activateModalWidget();

private:
    bool m_initialized : 1;

    TrayMessageType m_lastMessageType = MessageOptions;

    void *m_instanceMutex = nullptr;

    MainWindow *m_mainWindow = nullptr; // dummy window for tray icon

    NativeEventFilter *m_nativeEventFilter = nullptr;
    HotKeyManager *m_hotKeyManager = nullptr;

    TrayIcon *m_trayIcon = nullptr;

    ProgramsWindow *m_progWindow = nullptr;
    OptionsWindow *m_optWindow = nullptr;
    StatisticsWindow *m_statWindow = nullptr;
    ZonesWindow *m_zoneWindow = nullptr;
    GraphWindow *m_graphWindow = nullptr;

    UserSettings *m_userSettings = nullptr;
    RpcManager *m_rpcManager = nullptr;

    ConfManager *m_confManager = nullptr;
    QuotaManager *m_quotaManager = nullptr;
    StatManager *m_statManager = nullptr;
    DriverManager *m_driverManager = nullptr;
    AppInfoManager *m_appInfoManager = nullptr;
    LogManager *m_logManager = nullptr;
    TaskManager *m_taskManager = nullptr;

    AppInfoCache *m_appInfoCache = nullptr;
    HostInfoCache *m_hostInfoCache = nullptr;

    AppListModel *m_appListModel = nullptr;
    ZoneListModel *m_zoneListModel = nullptr;
};

#endif // FORTMANAGER_H
