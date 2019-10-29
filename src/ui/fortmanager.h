#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "mainwindow.h"
#include "util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(QQmlApplicationEngine)
QT_FORWARD_DECLARE_CLASS(QSystemTrayIcon)

QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(DriverManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(GraphWindow)
QT_FORWARD_DECLARE_CLASS(HotKeyManager)
QT_FORWARD_DECLARE_CLASS(LogManager)
QT_FORWARD_DECLARE_CLASS(NativeEventFilter)
QT_FORWARD_DECLARE_CLASS(QuotaManager)
QT_FORWARD_DECLARE_CLASS(StatManager)
QT_FORWARD_DECLARE_CLASS(TaskManager)
QT_FORWARD_DECLARE_CLASS(WidgetWindowStateWatcher)
QT_FORWARD_DECLARE_CLASS(WindowStateWatcher)

class FortManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FortSettings *fortSettings READ fortSettings CONSTANT)
    Q_PROPERTY(FirewallConf *firewallConf READ firewallConf NOTIFY firewallConfToEditChanged)
    Q_PROPERTY(FirewallConf *firewallConfToEdit READ firewallConfToEdit NOTIFY firewallConfToEditChanged)
    Q_PROPERTY(LogManager *logManager READ logManager CONSTANT)
    Q_PROPERTY(TaskManager *taskManager READ taskManager CONSTANT)

public:
    explicit FortManager(FortSettings *fortSettings,
                         QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    FortSettings *fortSettings() const { return m_fortSettings; }
    FirewallConf *firewallConf() const { return m_firewallConf; }
    FirewallConf *firewallConfToEdit() const { return m_firewallConfToEdit; }

    ConfManager *confManager() const { return m_confManager; }
    DriverManager *driverManager() const { return m_driverManager; }
    LogManager *logManager() const { return m_logManager; }
    TaskManager *taskManager() const { return m_taskManager; }

signals:
    void firewallConfToEditChanged();

    void afterSaveWindowState();
    void afterRestoreWindowState();

public slots:
    void installDriver();
    void removeDriver();

    void launch();

    void showTrayIcon();
    void showTrayMessage(const QString &message);
    void showTrayMenu(QMouseEvent *event);

    void showWindow();
    void closeWindow();

    void showGraphWindow();
    void closeGraphWindow(bool storeVisibility = false);
    void switchGraphWindow();
    void updateGraphWindow();

    void exit(int retcode = 0);

    bool checkPassword();

    void showErrorBox(const QString &text,
                      const QString &title = QString());
    void showInfoBox(const QString &text,
                     const QString &title = QString());

    QStringList getOpenFileNames(const QString &title = QString(),
                                 const QString &filter = QString());

    bool saveOriginConf(const QString &message);
    bool saveConf(bool onlyFlags = false);
    bool applyConf(bool onlyFlags = false);
    bool applyConfImmediateFlags();

    void setLanguage(int language);

private slots:
    void saveTrayFlags();

private:
    void setFirewallConfToEdit(FirewallConf *conf);

    static void registerQmlTypes();

    void setupThreadPool();

    bool setupDriver();
    void closeDriver();

    void setupLogManager();
    void closeLogManager();

    void setupStatManager();
    void setupConfManager();

    void setupLogger();

    void setupTaskManager();
    void setupTranslationManager();

    void setupTrayIcon();

    void setupAppInfoCache();

    bool setupEngine();
    void closeEngine();

    void closeUi();

    bool loadSettings(FirewallConf *conf);
    bool saveSettings(FirewallConf *newConf, bool onlyFlags = false,
                      bool immediateFlags = false);

    bool updateDriverConf(FirewallConf *conf, bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

    void saveWindowState();
    void restoreWindowState();

    void saveGraphWindowState(bool visible);
    void restoreGraphWindowState();

    void updateLogger();
    void updateTrayMenu();

    void addHotKey(QAction *action, const QString &shortcutText,
                   bool hotKeyEnabled);
    void removeHotKeys();

    static QAction *addAction(QWidget *widget,
                              const QIcon &icon, const QString &text,
                              const QObject *receiver = nullptr, const char *member = nullptr,
                              bool checkable = false, bool checked = false);
    static void setActionCheckable(QAction *action, bool checked = false,
                                   const QObject *receiver = nullptr, const char *member = nullptr);

private:
    MainWindow m_window;  // dummy window for tray icon

    QSystemTrayIcon *m_trayIcon;
    QQmlApplicationEngine *m_engine;

    QWindow *m_appWindow;
    WindowStateWatcher *m_appWindowState;

    GraphWindow *m_graphWindow;
    WidgetWindowStateWatcher *m_graphWindowState;

    FortSettings *m_fortSettings;
    FirewallConf *m_firewallConf;
    FirewallConf *m_firewallConfToEdit;

    QAction *m_graphWindowAction;
    QAction *m_filterEnabledAction;
    QAction *m_stopTrafficAction;
    QAction *m_stopInetTrafficAction;
    QList<QAction *> m_appGroupActions;

    QuotaManager *m_quotaManager;
    StatManager *m_statManager;
    ConfManager *m_confManager;
    DriverManager *m_driverManager;
    LogManager *m_logManager;
    NativeEventFilter *m_nativeEventFilter;
    HotKeyManager *m_hotKeyManager;
    TaskManager *m_taskManager;
    AppInfoCache *m_appInfoCache;
};

#endif // FORTMANAGER_H
