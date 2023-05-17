#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QActionGroup)
QT_FORWARD_DECLARE_CLASS(QTimer)

class ClickableMenu;
class ConfManager;
class DriverManager;
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
        CtrlSingleClick,
        AltSingleClick,
        DoubleClick,
        MiddleClick,
        RightClick,
        ClickTypeCount
    };
    enum ActionType : qint8 {
        ActionNone = -1,
        ActionShowHome = 0,
        ActionShowPrograms,
        ActionShowOptions,
        ActionShowStatistics,
        ActionShowTrafficGraph,
        ActionSwitchFilterEnabled,
        ActionSwitchStopTraffic,
        ActionSwitchStopInetTraffic,
        ActionShowFilterModeMenu,
        ActionShowTrayMenu,
        ActionIgnore,
        ActionTypeCount
    };

    explicit TrayIcon(QObject *parent = nullptr);

    TrayController *ctrl() const { return m_ctrl; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    HotKeyManager *hotKeyManager() const;
    DriverManager *driverManager() const;
    WindowManager *windowManager() const;

    QMenu *menu() const { return m_menu; }

    static ActionType clickEventActionType(IniUser *iniUser, ClickType clickType);
    static void setClickEventActionType(
            IniUser *iniUser, ClickType clickType, ActionType actionType);

public slots:
    void updateTrayIcon(bool alerted = false);

    void showTrayMenu(const QPoint &pos);
    void updateTrayMenu(bool onlyFlags = false);

    void quitProgram();

protected slots:
    void switchTrayMenu(bool checked);
    void switchFilterModeMenu(bool checked);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void saveTrayFlags();

    void switchTrayFlag(bool checked);
    void switchFilterMode(QAction *action);

private:
    void setupController();

    void retranslateUi();
    void retranslateFilterModeActions();

    void setupUi();
    void setupTrayMenu();
    void setupTrayMenuOptions();
    void setupTrayMenuFilterMode();

    void updateTrayMenuFlags();
    void updateAppGroupActions();

    void updateAlertTimer();
    void updateTrayIconShape();

    void addHotKey(QAction *action, const QString &shortcutText);
    void updateHotKeys();
    void removeHotKeys();

    void updateClickActions();

    QAction *clickAction(TrayIcon::ClickType clickType) const;
    QAction *clickActionFromIni(TrayIcon::ClickType clickType) const;
    QAction *clickActionByType(TrayIcon::ActionType actionType) const;

    void onMouseClicked(TrayIcon::ClickType clickType);
    void onTrayActivatedByTrigger();
    void onTrayActivatedByClick(TrayIcon::ClickType clickType, bool checkTriggered = false);

    void onWindowVisibilityChanged(quint32 code, bool isVisible);

private:
    bool m_trayTriggered : 1;
    bool m_alerted : 1;
    bool m_animatedAlert : 1;

    TrayController *m_ctrl = nullptr;

    QMenu *m_menu = nullptr;
    QAction *m_homeAction = nullptr;
    QAction *m_programsAction = nullptr;
    ClickableMenu *m_optionsMenu = nullptr;
    QAction *m_optionsAction = nullptr;
    QAction *m_policiesAction = nullptr;
    QAction *m_statisticsAction = nullptr;
    QAction *m_graphAction = nullptr;
    QAction *m_zonesAction = nullptr;
    QAction *m_filterEnabledAction = nullptr;
    QAction *m_stopTrafficAction = nullptr;
    QAction *m_stopInetTrafficAction = nullptr;
    QAction *m_filterModeMenuAction = nullptr;
    QMenu *m_filterModeMenu = nullptr;
    QActionGroup *m_filterModeActions = nullptr;
    QAction *m_quitAction = nullptr;
    QAction *m_trayMenuAction = nullptr;
    QList<QAction *> m_appGroupActions;

    QAction *m_clickActions[ClickTypeCount] = { nullptr };

    QTimer *m_alertTimer = nullptr;
};

#endif // TRAYICON_H
