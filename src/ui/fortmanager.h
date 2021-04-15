#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "util/classhelpers.h"

class AppInfoCache;
class AppListModel;
class AppStatModel;
class ConfManager;
class ConnListModel;
class ConnectionsWindow;
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
class StatManager;
class TaskManager;
class TrayIcon;
class WidgetWindowStateWatcher;
class ZoneListModel;
class ZonesWindow;

class FortManager : public QObject
{
    Q_OBJECT

public:
    enum TrayMessageType { MessageOptions, MessageZones };
    Q_ENUM(TrayMessageType)

    explicit FortManager(FortSettings *settings, EnvManager *envManager, QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    bool checkRunningInstance();

    void initialize();

    MainWindow *mainWindow() const { return m_mainWindow; }
    HotKeyManager *hotKeyManager() const { return m_hotKeyManager; }

    ProgramsWindow *progWindow() const { return m_progWindow; }
    OptionsWindow *optWindow() const { return m_optWindow; }
    ZonesWindow *zoneWindow() const { return m_zoneWindow; }
    GraphWindow *graphWindow() const { return m_graphWindow; }
    ConnectionsWindow *connWindow() const { return m_connWindow; }

    FirewallConf *conf() const;
    FirewallConf *confToEdit() const;

    FortSettings *settings() const { return m_settings; }
    EnvManager *envManager() const { return m_envManager; }
    ConfManager *confManager() const { return m_confManager; }
    DriverManager *driverManager() const { return m_driverManager; }
    LogManager *logManager() const { return m_logManager; }
    TaskManager *taskManager() const { return m_taskManager; }
    AppListModel *appListModel() const { return m_appListModel; }
    AppStatModel *appStatModel() const { return m_appStatModel; }
    ZoneListModel *zoneListModel() const { return m_zoneListModel; }
    ConnListModel *connListModel() const { return m_connListModel; }

signals:
    void optWindowChanged(bool visible);
    void graphWindowChanged(bool visible);

    void afterSaveProgWindowState();
    void afterRestoreProgWindowState();

    void afterSaveOptWindowState();
    void afterRestoreOptWindowState();

    void afterSaveZoneWindowState();
    void afterRestoreZoneWindowState();

    void afterSaveConnWindowState();
    void afterRestoreConnWindowState();

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

    void showZonesWindow();
    void closeZonesWindow();

    void showGraphWindow();
    void closeGraphWindow(bool storeVisibility = false);
    void switchGraphWindow();
    void updateGraphWindow();

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

    bool saveOriginConf(const QString &message, bool onlyFlags = false);
    bool saveConf(bool onlyFlags = false);
    bool applyConf(bool onlyFlags = false);
    bool applyConfImmediateFlags();

private:
    void setupTranslationManager();

    void setupThreadPool();

    bool setupDriver();
    void closeDriver();

    void setupAppInfoCache();
    void setupHostInfoCache();
    void setupModels();

    void setupLogManager();
    void closeLogManager();

    void setupEventFilter();
    void setupEnvManager();
    void setupStatManager();
    void setupConfManager();

    void setupLogger();
    void setupTaskManager();

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

    bool loadConf();
    bool saveConf(FirewallConf *newConf, bool onlyFlags = false);

    bool updateDriverConf(bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

    void saveProgWindowState();
    void restoreProgWindowState();

    void saveOptWindowState();
    void restoreOptWindowState();

    void saveZoneWindowState();
    void restoreZoneWindowState();

    void saveGraphWindowState(bool visible);
    void restoreGraphWindowState();

    void saveConnWindowState();
    void restoreConnWindowState();

    void onTrayActivated(int reason);
    void onTrayMessageClicked();

    QWidget *focusWidget() const;

private:
    bool m_trayTriggered : 1;

    TrayMessageType m_lastMessageType = MessageOptions;

    void *m_instanceMutex = nullptr;

    MainWindow *m_mainWindow = nullptr; // dummy window for tray icon

    NativeEventFilter *m_nativeEventFilter = nullptr;
    HotKeyManager *m_hotKeyManager = nullptr;

    TrayIcon *m_trayIcon = nullptr;

    ProgramsWindow *m_progWindow = nullptr;
    WidgetWindowStateWatcher *m_progWindowState = nullptr;

    OptionsWindow *m_optWindow = nullptr;
    WidgetWindowStateWatcher *m_optWindowState = nullptr;

    ZonesWindow *m_zoneWindow = nullptr;
    WidgetWindowStateWatcher *m_zoneWindowState = nullptr;

    GraphWindow *m_graphWindow = nullptr;
    WidgetWindowStateWatcher *m_graphWindowState = nullptr;

    ConnectionsWindow *m_connWindow = nullptr;
    WidgetWindowStateWatcher *m_connWindowState = nullptr;

    FortSettings *m_settings = nullptr;
    EnvManager *m_envManager = nullptr;
    QuotaManager *m_quotaManager = nullptr;
    StatManager *m_statManager = nullptr;
    DriverManager *m_driverManager = nullptr;
    ConfManager *m_confManager = nullptr;
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
