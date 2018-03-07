#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "mainwindow.h"

QT_FORWARD_DECLARE_CLASS(QQmlApplicationEngine)
QT_FORWARD_DECLARE_CLASS(QSystemTrayIcon)

QT_FORWARD_DECLARE_CLASS(DatabaseManager)
QT_FORWARD_DECLARE_CLASS(DriverManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(HotKeyManager)
QT_FORWARD_DECLARE_CLASS(LogManager)
QT_FORWARD_DECLARE_CLASS(NativeEventFilter)
QT_FORWARD_DECLARE_CLASS(QuotaManager)
QT_FORWARD_DECLARE_CLASS(TaskManager)
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
    virtual ~FortManager();

    FortSettings *fortSettings() const { return m_fortSettings; }
    FirewallConf *firewallConf() const { return m_firewallConf; }
    FirewallConf *firewallConfToEdit() const { return m_firewallConfToEdit; }

    DriverManager *driverManager() const { return m_driverManager; }
    LogManager *logManager() const { return m_logManager; }
    TaskManager *taskManager() const { return m_taskManager; }

signals:
    void firewallConfToEditChanged();

public slots:
    void showTrayIcon();
    void showTrayMessage(const QString &message);

    void showWindow();
    void closeWindow();

    void exit(int retcode = 0);

    bool checkPassword();

    void showErrorBox(const QString &text,
                      const QString &title = QString());
    void showInfoBox(const QString &text,
                     const QString &title = QString());

    bool saveOriginConf(const QString &message);
    bool saveConf(bool onlyFlags = false);
    bool applyConf(bool onlyFlags = false);
    bool applyConfImmediateFlags();

    void setLanguage(int language);

private slots:
    void saveTrayFlags();

private:
    FirewallConf *nullConf() const { return nullptr; }

    void setFirewallConfToEdit(FirewallConf *conf);

    static void registerQmlTypes();

    bool setupDriver();
    void closeDriver();

    void setupDatabaseManager();

    void setupLogger();
    void setupLogManager();

    void setupTrayIcon();
    bool setupEngine();

    bool loadSettings(FirewallConf *conf);
    bool saveSettings(FirewallConf *newConf, bool onlyFlags = false,
                      bool immediateFlags = false);

    bool updateDriverConf(FirewallConf *conf);
    bool updateDriverConfFlags(FirewallConf *conf);

    void updateLogManager(bool active);
    void updateDatabaseManager(FirewallConf *conf);

    FirewallConf *cloneConf(const FirewallConf &conf);

    void saveWindowState();
    void restoreWindowState();

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

    FortSettings *m_fortSettings;
    FirewallConf *m_firewallConf;
    FirewallConf *m_firewallConfToEdit;

    QAction *m_filterEnabledAction;
    QAction *m_stopTrafficAction;
    QAction *m_stopInetTrafficAction;
    QList<QAction *> m_appGroupActions;

    QuotaManager *m_quotaManager;
    DatabaseManager *m_databaseManager;
    DriverManager *m_driverManager;
    LogManager *m_logManager;
    NativeEventFilter *m_nativeEventFilter;
    HotKeyManager *m_hotKeyManager;
    TaskManager *m_taskManager;
};

#endif // FORTMANAGER_H
