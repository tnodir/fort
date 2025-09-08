#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

#include <form/form_types.h>

#include "trayicon_types.h"

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QActionGroup)
QT_FORWARD_DECLARE_CLASS(QTimer)

class ConfAppManager;
class ConfManager;
class ConfRuleManager;
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
    explicit TrayIcon(QObject *parent = nullptr);

    QString iconPath() const { return m_iconPath; }

    QMenu *menu() const { return m_menu; }
    QMenu *optionsMenu() const { return m_optionsMenu; }

    TrayController *ctrl() const { return m_ctrl; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    ConfAppManager *confAppManager() const;
    ConfRuleManager *confRuleManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    HotKeyManager *hotKeyManager() const;
    DriverManager *driverManager() const;
    WindowManager *windowManager() const;

    static tray::ActionType clickEventActionType(IniUser *iniUser, tray::ClickType clickType);
    static void setClickEventActionType(
            IniUser *iniUser, tray::ClickType clickType, tray::ActionType actionType);
    static void resetClickEventActionType(IniUser *iniUser, tray::ClickType clickType);

signals:
    void iconPathChanged(const QString &iconPath);

public slots:
    void updateTrayIcon(bool alerted = false);

    void showTrayMessage(const QString &message, tray::MessageType type = tray::MessageOptions);

    void showTrayMenu(const QPoint &pos);
    void hideTrayMenuLater();

    void updateTrayMenu(bool onlyFlags = false);

    void quitProgram();

    void processMouseClick(Qt::MouseButton button, Qt::KeyboardModifiers modifiers);

protected slots:
    void setupByIniUser(const IniUser &ini, bool onlyFlags);

    void switchTrayMenu(bool checked);
    void switchBlockTrafficMenu(bool checked);
    void switchFilterModeMenu(bool checked);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void onTrayMessageClicked();

    void onAppAlerted(bool alerted = true);

    void showProgramsOrAlertWindow();

    void saveTrayFlags();

    void onShowWindowAction();

    void switchTrayFlag(bool checked);
    void switchBlockTraffic(QAction *action);
    void switchFilterMode(QAction *action);

    void switchTrayRuleFlag(bool checked);

private:
    void setIconPath(const QString &v);

    void setupController();

    void retranslateUi();
    void retranslateBlockTrafficActions();
    void retranslateFilterModeActions();

    void setupUi();
    void setupTrayMenu();
    void setupTrayMenuTopActions();
    void setupTrayMenuOptions();
    void setupTrayMenuBlockTraffic();
    void setupTrayMenuFilterMode();
    void setupTrayMenuGroupActions();
    void setupTrayMenuRuleActions();
    void setupTrayMenuBottomActions();

    void updateTrayMenuFlags();
    void updateAppGroupActions();
    void updateRuleActions();

    void updateBlockTrafficMenuIcon(int index);
    void updateFilterModeMenuIcon(int index);

    void sendAlertMessage();
    void updateAlertTimer();

    void setupAlertTimer();
    void removeAlertTimer();

    void updateTrayIconShape();

    QIcon getTrayIcon() const;

    QString trayIconPath(bool &isDefault) const;
    QString trayIconBlockPath(int blockType, bool &isDefault) const;

    void updateActionHotKeys();

    void addHotKey(QAction *action, const char *iniKey);
    void updateHotKeys();

    void updateClickActions();

    QAction *clickAction(tray::ClickType clickType) const;
    QAction *clickActionFromIni(tray::ClickType clickType) const;
    QAction *clickActionByType(tray::ActionType actionType) const;

    void onMouseClicked(
            tray::ClickType clickType, tray::ClickType menuClickType = tray::RightClick);
    void onTrayActivatedByTrigger();
    void onTrayActivatedByClick(tray::ClickType clickType);

    void onWindowVisibilityChanged(WindowCode code, bool isVisible);

private:
    bool m_trayTriggered : 1 = false;
    bool m_alerted : 1 = false;
    bool m_animatedAlert : 1 = false;

    tray::MessageType m_lastTrayMessageType = tray::MessageOptions;

    QString m_iconPath;

    TrayController *m_ctrl = nullptr;

    QMenu *m_menu = nullptr;
    QAction *m_homeAction = nullptr;
    QAction *m_programsAction = nullptr;
    QAction *m_programsOrAlertAction = nullptr;
    QMenu *m_optionsMenu = nullptr;
    QAction *m_optionsAction = nullptr;
    QAction *m_rulesAction = nullptr;
    QAction *m_zonesAction = nullptr;
    QAction *m_groupsAction = nullptr;
    QAction *m_servicesAction = nullptr;
    QAction *m_statisticsAction = nullptr;
    QAction *m_graphAction = nullptr;
    QAction *m_filterEnabledAction = nullptr;
    QAction *m_snoozeAlertsAction = nullptr;
    QAction *m_blockTrafficMenuAction = nullptr;
    QMenu *m_blockTrafficMenu = nullptr;
    QActionGroup *m_blockTrafficActions = nullptr;
    QAction *m_filterModeMenuAction = nullptr;
    QMenu *m_filterModeMenu = nullptr;
    QActionGroup *m_filterModeActions = nullptr;
    QAction *m_quitAction = nullptr;
    QAction *m_trayMenuAction = nullptr;
    QList<QAction *> m_appGroupActions;
    QList<QAction *> m_ruleActions;
    QVector<const char *> m_actionIniKeys;

    QAction *m_clickActions[tray::ClickTypeCount] = { nullptr };

    QTimer *m_alertTimer = nullptr;
};

#endif // TRAYICON_H
