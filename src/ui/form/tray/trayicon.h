#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QAction)

class ConfManager;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class IniUser;
class HotKeyManager;
class TrayController;

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    explicit TrayIcon(FortManager *fortManager, QObject *parent = nullptr);
    ~TrayIcon() override;

    TrayController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    HotKeyManager *hotKeyManager() const;

signals:
    void mouseClicked();
    void mouseDoubleClicked();
    void mouseMiddleClicked();
    void mouseRightClicked(const QPoint &pos);

public slots:
    void updateTrayIcon(bool alerted = false);

    void showTrayMenu(const QPoint &pos);
    void updateTrayMenu(bool onlyFlags = false);

protected slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void saveTrayFlags();

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    void setupTrayMenu();

    void updateTrayMenuFlags();
    void updateAppGroupActions();

    void addHotKey(QAction *action, const QString &shortcutText);
    void updateHotKeys();
    void removeHotKeys();

private:
    bool m_trayTriggered : 1;

    QMenu *m_menu = nullptr;
    QAction *m_programsAction = nullptr;
    QAction *m_optionsAction = nullptr;
    QAction *m_statisticsAction = nullptr;
    QAction *m_graphAction = nullptr;
    QAction *m_zonesAction = nullptr;
    QAction *m_filterEnabledAction = nullptr;
    QAction *m_stopTrafficAction = nullptr;
    QAction *m_stopInetTrafficAction = nullptr;
    QAction *m_allowAllNewAction = nullptr;
    QAction *m_quitAction = nullptr;
    QList<QAction *> m_appGroupActions;

    TrayController *m_ctrl = nullptr;
};

#endif // TRAYICON_H
