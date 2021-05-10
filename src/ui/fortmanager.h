#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "util/classhelpers.h"

class AppInfoCache;
class AppInfoManager;
class AppListModel;
class AppStatModel;
class ConfManager;
class ConnListModel;
class ConnectionsWindow;
class ControlManager;
class DriverManager;
class EnvManager;
class FirewallConf;
class FortSettings;
class GraphWindow;
class HostInfoCache;
class HotKeyManager;
class LogManager;
class MainWindow;
class NativeEventFilter;
class OptionsWindow;
class ProgramsWindow;
class QuotaManager;
class RpcManager;
class StatManager;
class TaskManager;
class TrayIcon;
class ZoneListModel;
class ZonesWindow;

class FortManager : public QObject
{
    Q_OBJECT

public:
    enum TrayMessageType { MessageOptions, MessageZones };
    Q_ENUM(TrayMessageType)

    explicit FortManager(FortSettings *settings, EnvManager *envManager,
            ControlManager *controlManager, QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    MainWindow *mainWindow() const { return m_mainWindow; }
    HotKeyManager *hotKeyManager() const { return m_hotKeyManager; }

    ProgramsWindow *progWindow() const { return m_progWindow; }
    OptionsWindow *optWindow() const { return m_optWindow; }
    ZonesWindow *zoneWindow() const { return m_zoneWindow; }
    GraphWindow *graphWindow() const { return m_graphWindow; }
    ConnectionsWindow *connWindow() const { return m_connWindow; }

    FortSettings *settings() const { return m_settings; }
    EnvManager *envManager() const { return m_envManager; }
    ControlManager *controlManager() const { return m_controlManager; }
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
    AppStatModel *appStatModel() const { return m_appStatModel; }
    ZoneListModel *zoneListModel() const { return m_zoneListModel; }
    ConnListModel *connListModel() const { return m_connListModel; }

    bool checkRunningInstance();

    void initialize();

signals:
    void optWindowChanged(bool visible);
    void graphWindowChanged(bool visible);

public slots:
    void installDriver();
    void removeDriver();

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

    void showZonesWindow();
    void closeZonesWindow();

    void showGraphWindow();
    void closeGraphWindow(bool wasVisible = false);
    void switchGraphWindow();

    void showConnectionsWindow();
    void closeConnectionsWindow();

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

    void createManagers();

    void setupControlManager();
    void setupRpcManager();

    bool setupDriver();
    void closeDriver();

    void setupLogManager();
    void closeLogManager();

    void setupLogger();
    void updateLogger();

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

    void setupTranslationManager();

    void setupMainWindow();
    void closeMainWindow();

    void setupHotKeyManager();
    void setupTrayIcon();

    void setupProgramsWindow();
    void setupOptionsWindow();
    void setupZonesWindow();
    void setupGraphWindow();
    void setupConnectionsWindow();

    void closeUi();

    void loadConf();

    bool updateDriverConf(bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

    void onTrayActivated(int reason);
    void onTrayMessageClicked();

    QWidget *focusWidget() const;
    static void activateModalWidget();

private:
    bool m_initialized : 1;
    bool m_trayTriggered : 1;

    TrayMessageType m_lastMessageType = MessageOptions;

    void *m_instanceMutex = nullptr;

    MainWindow *m_mainWindow = nullptr; // dummy window for tray icon

    NativeEventFilter *m_nativeEventFilter = nullptr;
    HotKeyManager *m_hotKeyManager = nullptr;

    TrayIcon *m_trayIcon = nullptr;

    ProgramsWindow *m_progWindow = nullptr;
    OptionsWindow *m_optWindow = nullptr;
    ZonesWindow *m_zoneWindow = nullptr;
    GraphWindow *m_graphWindow = nullptr;
    ConnectionsWindow *m_connWindow = nullptr;

    FortSettings *m_settings = nullptr;
    EnvManager *m_envManager = nullptr;
    ControlManager *m_controlManager = nullptr;
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
    AppStatModel *m_appStatModel = nullptr;
    ZoneListModel *m_zoneListModel = nullptr;
    ConnListModel *m_connListModel = nullptr;
};

#endif // FORTMANAGER_H
