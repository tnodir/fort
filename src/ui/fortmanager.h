#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>
#include <QMainWindow>

class QQmlApplicationEngine;
class QSystemTrayIcon;

class FortSettings;
class FirewallConf;

class FortManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FortSettings *fortSettings READ fortSettings CONSTANT)
    Q_PROPERTY(FirewallConf *firewallConfToEdit READ firewallConfToEdit NOTIFY firewallConfToEditChanged)

public:
    explicit FortManager(QObject *parent = nullptr);

    FortSettings *fortSettings() const { return m_fortSettings; }

    FirewallConf *firewallConfToEdit() const {
        return m_firewallConfToEdit ? m_firewallConfToEdit : m_firewallConf;
    }

signals:
    void firewallConfToEditChanged();

public slots:
    void showTrayIcon();

    void showWindow();
    void closeWindow();

    bool saveConf();
    bool applyConf();

private slots:
    void saveTrayFlags();

private:
    FirewallConf *nullConf() const { return nullptr; }

    void setFirewallConfToEdit(FirewallConf *conf);

    static void registerQmlTypes();

    void setupTrayIcon();
    void setupEngine();

    bool saveSettings(FirewallConf *newConf);

    FirewallConf *cloneConf(const FirewallConf &conf);

    void updateTrayMenu();

    static QAction *addAction(QWidget *widget,
                              const QIcon &icon, const QString &text,
                              const QObject *receiver = 0, const char *member = 0,
                              bool checkable = false, bool checked = false);
    static void setActionCheckable(QAction *action, bool checked = false,
                                   const QObject *receiver = 0, const char *member = 0);

private:
    QMainWindow m_window;  // dummy window for tray icon

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
    QMenu *m_appGroupsMenu;
};

#endif // FORTMANAGER_H
