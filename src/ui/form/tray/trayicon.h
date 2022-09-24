#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QActionGroup)

class ConfManager;
class FirewallConf;
class FortSettings;
class HotKeyManager;
class IniOptions;
class IniUser;
class TrayController;
class WindowManager;

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    enum ClickType : qint8 {
        SingleClick = 0,
        DoubleClick,
        MiddleClick,
        RightClick,
        ClickTypeCount
    };
    enum ActionType : qint8 {
        ActionNone = -1,
        ActionShowPrograms = 0,
        ActionShowOptions,
        ActionShowStatistics,
        ActionShowTrafficGraph,
        ActionSwitchFilterEnabled,
        ActionSwitchStopTraffic,
        ActionSwitchStopInetTraffic,
        ActionSwitchAutoAllowPrograms,
        ActionShowTrayMenu,
        ActionIgnore
    };

    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

    TrayController *ctrl() const { return m_ctrl; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    HotKeyManager *hotKeyManager() const;
    WindowManager *windowManager() const;

    ActionType clickEventActionType(ClickType clickType) const;
    void setClickEventActionType(ClickType clickType, ActionType actionType);

public slots:
    void updateTrayIcon(bool alerted = false);

    void showTrayMenu(const QPoint &pos);
    void updateTrayMenu(bool onlyFlags = false);

protected slots:
    void switchTrayMenu(bool checked);

    void onMouseClicked(TrayIcon::ClickType clickType);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void saveTrayFlags();

    void switchTrayFlag(bool checked);
    void switchFilterMode(QAction *action);
    void quitProgram();

private:
    void setupController();

    void retranslateUi();
    void retranslateFilterModeActions();

    void setupUi();
    void setupTrayMenu();
    void setupTrayMenuFilterMode();

    void updateTrayMenuFlags();
    void updateAppGroupActions();

    void addHotKey(QAction *action, const QString &shortcutText);
    void updateHotKeys();
    void removeHotKeys();

    void updateClickActions();

    QAction *clickAction(ClickType clickType) const;
    QAction *clickActionFromIni(ClickType clickType) const;
    QAction *clickActionByType(ActionType actionType) const;

private:
    bool m_trayTriggered : 1;

    TrayController *m_ctrl = nullptr;

    QMenu *m_menu = nullptr;
    QAction *m_programsAction = nullptr;
    QAction *m_optionsAction = nullptr;
    QAction *m_statisticsAction = nullptr;
    QAction *m_graphAction = nullptr;
    QAction *m_zonesAction = nullptr;
    QAction *m_filterEnabledAction = nullptr;
    QAction *m_stopTrafficAction = nullptr;
    QAction *m_stopInetTrafficAction = nullptr;
    QAction *m_autoAllowProgsAction = nullptr;
    QAction *m_trayMenuAction = nullptr;
    QMenu *m_filterModeMenu = nullptr;
    QActionGroup *m_filterModeActions = nullptr;
    QAction *m_quitAction = nullptr;
    QList<QAction *> m_appGroupActions;

    QAction *m_clickActions[ClickTypeCount] = { nullptr };
};

#endif // TRAYICON_H
