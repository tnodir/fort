#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)

class FirewallConf;
class FortManager;
class FortSettings;
class HotKeyManager;
class TrayController;

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    explicit TrayIcon(FortManager *fortManager, QObject *parent = nullptr);

    TrayController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    FirewallConf *conf() const;
    HotKeyManager *hotKeyManager() const;

public slots:
    void updateTrayIcon(bool alerted = false);

    void showTrayMenu(QMouseEvent *event);
    void updateTrayMenu(bool onlyFlags = false);
    void updateTrayMenuFlags();

protected slots:
    void retranslateTrayMenu();

    void onRetranslateUi();

    void saveTrayFlags();

private:
    void setupController();

    void setupUi();
    void setupTrayMenu();

    void updateAppGroupActions();

    void addHotKey(QAction *action, const QString &shortcutText);
    void updateHotKeys();
    void removeHotKeys();

private:
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

    TrayController *m_ctrl = nullptr;
};

#endif // TRAYICON_H
