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

class FortManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FortSettings *fortSettings READ fortSettings CONSTANT)
    Q_PROPERTY(FirewallConf *firewallConfToEdit READ firewallConfToEdit NOTIFY firewallConfToEditChanged)
    Q_PROPERTY(DriverManager *driverManager READ driverManager CONSTANT)

public:
    explicit FortManager(QObject *parent = nullptr);

    FortSettings *fortSettings() const { return m_fortSettings; }

    FirewallConf *firewallConfToEdit() const {
        return m_firewallConfToEdit ? m_firewallConfToEdit : m_firewallConf;
    }

    DriverManager *driverManager() const { return m_driverManager; }

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
    void setupEngine();

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
};

#endif // FORTMANAGER_H
