#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "mainwindow.h"

class QQmlApplicationEngine;
class QSystemTrayIcon;

class DriverManager;
class FortSettings;
class FirewallConf;
class LogBuffer;
class TaskManager;

class FortManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FortSettings *fortSettings READ fortSettings CONSTANT)
    Q_PROPERTY(FirewallConf *firewallConf READ firewallConf NOTIFY firewallConfToEditChanged)
    Q_PROPERTY(FirewallConf *firewallConfToEdit READ firewallConfToEdit NOTIFY firewallConfToEditChanged)
    Q_PROPERTY(DriverManager *driverManager READ driverManager CONSTANT)
    Q_PROPERTY(TaskManager *taskManager READ taskManager CONSTANT)

public:
    explicit FortManager(FortSettings *fortSettings,
                         QObject *parent = nullptr);

    FortSettings *fortSettings() const { return m_fortSettings; }
    FirewallConf *firewallConf() const { return m_firewallConf; }
    FirewallConf *firewallConfToEdit() const { return m_firewallConfToEdit; }

    DriverManager *driverManager() const { return m_driverManager; }
    TaskManager *taskManager() const { return m_taskManager; }

signals:
    void firewallConfToEditChanged();

public slots:
    void showTrayIcon();

    void showWindow();
    void closeWindow();

    void exit(int retcode = 0);

    void showErrorBox(const QString &text);

    bool saveConf(bool onlyFlags = false);
    bool applyConf(bool onlyFlags = false);

    void setAppLogBlocked(bool enable);

    void setLanguage(int language);

private slots:
    void saveTrayFlags();

private:
    FirewallConf *nullConf() const { return nullptr; }

    void setFirewallConfToEdit(FirewallConf *conf);

    static void registerQmlTypes();

    bool setupDriver();
    void setupTrayIcon();
    bool setupEngine();

    bool loadSettings(FirewallConf *conf);
    bool saveSettings(FirewallConf *newConf, bool onlyFlags = false);

    bool updateDriverConf(FirewallConf *conf);
    bool updateDriverConfFlags(FirewallConf *conf);

    FirewallConf *cloneConf(const FirewallConf &conf);

    void updateTrayMenu();

    static QAction *addAction(QWidget *widget,
                              const QIcon &icon, const QString &text,
                              const QObject *receiver = 0, const char *member = 0,
                              bool checkable = false, bool checked = false);
    static void setActionCheckable(QAction *action, bool checked = false,
                                   const QObject *receiver = 0, const char *member = 0);

private:
    MainWindow m_window;  // dummy window for tray icon

    QSystemTrayIcon *m_trayIcon;
    QQmlApplicationEngine *m_engine;
    QWindow *m_appWindow;

    FortSettings *m_fortSettings;
    FirewallConf *m_firewallConf;
    FirewallConf *m_firewallConfToEdit;

    QAction *m_filterEnabledAction;
    QAction *m_ipIncludeAllAction;
    QAction *m_ipExcludeAllAction;
    QAction *m_appBlockAllAction;
    QAction *m_appAllowAllAction;
    QList<QAction *> m_appGroupActions;

    DriverManager *m_driverManager;
    TaskManager *m_taskManager;
};

#endif // FORTMANAGER_H
