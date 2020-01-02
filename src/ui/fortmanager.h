#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "mainwindow.h"
#include "util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(QSystemTrayIcon)

QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(DriverManager)
QT_FORWARD_DECLARE_CLASS(EnvManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(GraphWindow)
QT_FORWARD_DECLARE_CLASS(HotKeyManager)
QT_FORWARD_DECLARE_CLASS(LogManager)
QT_FORWARD_DECLARE_CLASS(NativeEventFilter)
QT_FORWARD_DECLARE_CLASS(OptionsWindow)
QT_FORWARD_DECLARE_CLASS(ProgramsWindow)
QT_FORWARD_DECLARE_CLASS(QuotaManager)
QT_FORWARD_DECLARE_CLASS(StatManager)
QT_FORWARD_DECLARE_CLASS(TaskManager)
QT_FORWARD_DECLARE_CLASS(WidgetWindowStateWatcher)

class FortManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FortSettings *settings READ settings CONSTANT)
    Q_PROPERTY(FirewallConf *conf READ conf NOTIFY confToEditChanged)
    Q_PROPERTY(FirewallConf *confToEdit READ confToEdit NOTIFY confToEditChanged)
    Q_PROPERTY(LogManager *logManager READ logManager CONSTANT)
    Q_PROPERTY(TaskManager *taskManager READ taskManager CONSTANT)

public:
    explicit FortManager(FortSettings *settings,
                         QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    FortSettings *settings() const { return m_settings; }
    FirewallConf *conf() const { return m_conf; }
    FirewallConf *confToEdit() const { return m_confToEdit; }

    ConfManager *confManager() const { return m_confManager; }
    DriverManager *driverManager() const { return m_driverManager; }
    LogManager *logManager() const { return m_logManager; }
    TaskManager *taskManager() const { return m_taskManager; }

signals:
    void confToEditChanged();

    void afterSaveProgWindowState();
    void afterRestoreProgWindowState();

    void afterSaveOptWindowState();
    void afterRestoreOptWindowState();

public slots:
    void installDriver();
    void removeDriver();

    void launch();

    void showTrayIcon();
    void showTrayMessage(const QString &message);
    void showTrayMenu(QMouseEvent *event);

    void showProgramsWindow();
    void closeProgramsWindow();

    void showOptionsWindow();
    void closeOptionsWindow();

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
    bool showQuestionBox(const QString &text,
                         const QString &title = QString());

    bool saveOriginConf(const QString &message);
    bool saveConf(bool onlyFlags = false);
    bool applyConf(bool onlyFlags = false);
    bool applyConfImmediateFlags();

private slots:
    void saveTrayFlags();

private:
    void setConfToEdit(FirewallConf *conf);

    void setupThreadPool();

    bool setupDriver();
    void closeDriver();

    void setupLogManager();
    void closeLogManager();

    void setupEnvManager();
    void setupStatManager();
    void setupConfManager();

    void setupLogger();

    void setupTaskManager();
    void setupTranslationManager();

    void setupTrayIcon();

    void setupAppInfoCache();

    bool setupProgramsWindow();
    bool setupOptionsWindow();

    void closeUi();

    bool loadSettings();
    bool saveSettings(FirewallConf *newConf, bool onlyFlags = false,
                      bool immediateFlags = false);

    bool updateDriverConf(bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

    void saveProgWindowState();
    void restoreProgWindowState();

    void saveOptWindowState();
    void restoreOptWindowState();

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

    bool m_trayTriggered = false;
    QSystemTrayIcon *m_trayIcon = nullptr;

    ProgramsWindow *m_progWindow = nullptr;
    WidgetWindowStateWatcher *m_progWindowState = nullptr;

    OptionsWindow *m_optWindow = nullptr;
    WidgetWindowStateWatcher *m_optWindowState = nullptr;

    GraphWindow *m_graphWindow = nullptr;
    WidgetWindowStateWatcher *m_graphWindowState = nullptr;

    FortSettings *m_settings = nullptr;
    FirewallConf *m_conf = nullptr;
    FirewallConf *m_confToEdit = nullptr;

    QAction *m_graphWindowAction = nullptr;
    QAction *m_filterEnabledAction = nullptr;
    QAction *m_stopTrafficAction = nullptr;
    QAction *m_stopInetTrafficAction = nullptr;
    QList<QAction *> m_appGroupActions;

    QuotaManager *m_quotaManager = nullptr;
    StatManager *m_statManager = nullptr;
    ConfManager *m_confManager = nullptr;
    DriverManager *m_driverManager = nullptr;
    EnvManager *m_envManager = nullptr;
    LogManager *m_logManager = nullptr;
    NativeEventFilter *m_nativeEventFilter = nullptr;
    HotKeyManager *m_hotKeyManager = nullptr;
    TaskManager *m_taskManager = nullptr;
    AppInfoCache *m_appInfoCache = nullptr;
};

#endif // FORTMANAGER_H
