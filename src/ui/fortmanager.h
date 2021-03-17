#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "form/controls/mainwindow.h"
#include "util/classhelpers.h"

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

    explicit FortManager(FortSettings *settings, QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    FirewallConf *conf() const;
    FirewallConf *confToEdit() const;

    FortSettings *settings() const { return m_settings; }
    ConfManager *confManager() const { return m_confManager; }
    DriverManager *driverManager() const { return m_driverManager; }
    EnvManager *envManager() const { return m_envManager; }
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

    void launch();

    void showTrayIcon();
    void showTrayMessage(
            const QString &message, FortManager::TrayMessageType type = MessageOptions);
    void showTrayMenu(QMouseEvent *event);

    void showProgramsWindow();
    void closeProgramsWindow();

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

    void exit(int retcode = 0);

    bool checkPassword();

    void showErrorBox(const QString &text, const QString &title = QString());
    void showInfoBox(const QString &text, const QString &title = QString());
    bool showQuestionBox(const QString &text, const QString &title = QString());

    bool saveOriginConf(const QString &message);
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
    MainWindow m_window; // dummy window for tray icon

    TrayMessageType m_lastMessageType = MessageOptions;

    bool m_trayTriggered = false;
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
    QuotaManager *m_quotaManager = nullptr;
    StatManager *m_statManager = nullptr;
    DriverManager *m_driverManager = nullptr;
    EnvManager *m_envManager = nullptr;
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
