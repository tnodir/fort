#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QSystemTrayIcon)

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
    void optWindowChanged();

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
    void showTrayMessage(
            const QString &message, FortManager::TrayMessageType type = MessageOptions);
    void showTrayMenu(QMouseEvent *event);

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
    bool isPasswordRequired();

    void showErrorBox(const QString &text, const QString &title = QString());
    void showInfoBox(const QString &text, const QString &title = QString());
    bool showQuestionBox(const QString &text, const QString &title = QString());
    bool showYesNoBox(const QString &text, const QString &yesText, const QString &noText,
            const QString &title = QString());

    bool saveOriginConf(const QString &message, bool onlyFlags = false);
    bool saveConf(bool onlyFlags = false);
    bool applyConf(bool onlyFlags = false);
    bool applyConfImmediateFlags();

private slots:
    void saveTrayFlags();

private:
    void setupTranslationManager();

    void setupThreadPool();

    bool setupDriver();
    void closeDriver();

    void setupModels();

    void setupLogManager();
    void closeLogManager();

    void setupEnvManager();
    void setupStatManager();
    void setupConfManager();

    void setupLogger();
    void setupTaskManager();

    void setupTrayIcon();

    void setupAppInfoCache();

    bool setupProgramsWindow();
    bool setupOptionsWindow();
    bool setupZonesWindow();
    bool setupConnectionsWindow();

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

    void updateTrayIcon(bool alerted = false);

    void updateTrayMenu(bool onlyFlags = false);
    void createTrayMenu();
    void updateTrayMenuFlags();
    void retranslateTrayMenu();

    void onTrayActivated(int reason);
    void onTrayMessageClicked();

    void addHotKey(QAction *action, const QString &shortcutText, bool hotKeyEnabled);
    void removeHotKeys();

    QWidget *focusWidget();

    static QAction *addAction(QWidget *widget, const QIcon &icon, const QString &text,
            const QObject *receiver = nullptr, const char *member = nullptr, bool checkable = false,
            bool checked = false);
    static void setActionCheckable(QAction *action, bool checked = false,
            const QObject *receiver = nullptr, const char *member = nullptr);

private:
    bool m_trayTriggered = false;

    TrayMessageType m_lastMessageType = MessageOptions;

    void *m_instanceMutex = nullptr;

    MainWindow *m_mainWindow = nullptr; // dummy window for tray icon

    QSystemTrayIcon *m_trayIcon = nullptr;

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

    QAction *m_programsAction = nullptr;
    QAction *m_optionsAction = nullptr;
    QAction *m_zonesAction = nullptr;
    QAction *m_graphWindowAction = nullptr;
    QAction *m_connectionsAction = nullptr;
    QAction *m_filterEnabledAction = nullptr;
    QAction *m_stopTrafficAction = nullptr;
    QAction *m_stopInetTrafficAction = nullptr;
    QAction *m_allowAllNewAction = nullptr;
    QAction *m_quitAction = nullptr;
    QList<QAction *> m_appGroupActions;

    FortSettings *m_settings = nullptr;
    EnvManager *m_envManager = nullptr;
    QuotaManager *m_quotaManager = nullptr;
    StatManager *m_statManager = nullptr;
    DriverManager *m_driverManager = nullptr;
    ConfManager *m_confManager = nullptr;
    LogManager *m_logManager = nullptr;
    NativeEventFilter *m_nativeEventFilter = nullptr;
    HotKeyManager *m_hotKeyManager = nullptr;
    TaskManager *m_taskManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;
    HostInfoCache *m_hostInfoCache = nullptr;

    AppListModel *m_appListModel = nullptr;
    AppStatModel *m_appStatModel = nullptr;
    ZoneListModel *m_zoneListModel = nullptr;
    ConnListModel *m_connListModel = nullptr;
};

#endif // FORTMANAGER_H
