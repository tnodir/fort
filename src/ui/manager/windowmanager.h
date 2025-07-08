#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QMessageBox>
#include <QObject>

#include <form/form_types.h>
#include <util/ioc/iocservice.h>
#include <util/taskbarbutton.h>

class FormWindow;
class FortSettings;
class GraphWindow;
class GroupsWindow;
class HomeWindow;
class IniUser;
class MainWindow;
class OptionsWindow;
class ProgramAlertWindow;
class ProgramsWindow;
class RulesWindow;
class ServicesWindow;
class SpeedLimitsWindow;
class StatisticsWindow;
class TrayIcon;
class WidgetWindow;
class ZonesWindow;

class WindowManager : public QObject, public IocService
{
    Q_OBJECT

public:
    enum TrayMessageType : qint8 {
        TrayMessageOptions,
        TrayMessageNewVersion,
        TrayMessageZones,
        TrayMessageAlert,
    };
    Q_ENUM(TrayMessageType)

    explicit WindowManager(QObject *parent = nullptr);

    MainWindow *mainWindow() const { return m_mainWindow; }
    HomeWindow *homeWindow() const { return m_homeWindow; }
    ProgramsWindow *progWindow() const { return m_progWindow; }
    ProgramAlertWindow *progAlertWindow() const { return m_progAlertWindow; }
    RulesWindow *rulesWindow() const { return m_rulesWindow; }
    OptionsWindow *optWindow() const { return m_optWindow; }
    StatisticsWindow *statWindow() const { return m_statWindow; }
    ServicesWindow *servicesWindow() const { return m_servicesWindow; }
    ZonesWindow *zonesWindow() const { return m_zonesWindow; }
    GroupsWindow *groupsWindow() const { return m_groupsWindow; }
    GraphWindow *graphWindow() const { return m_graphWindow; }
    TrayIcon *trayIcon() const { return m_trayIcon; }

    TaskbarButton &taskbarButton() { return m_taskbarButton; }

    void setUp() override;
    void tearDown() override;

    void initialize();

    bool isWindowOpen(WindowCode code) const { return isAnyWindowOpen(code); }

    static QFont defaultFont();

signals:
    void windowVisibilityChanged(WindowCode code, bool isVisible);

public slots:
    void setupTrayIcon();
    void showTrayIcon();
    void closeTrayIcon();
    void showTrayMessage(
            const QString &message, WindowManager::TrayMessageType type = TrayMessageOptions);

    void showSplashScreen();

    void showHomeWindow();
    void closeHomeWindow();
    void quitHomeWindow(QEvent *event);

    virtual bool exposeHomeWindow();
    void showHomeWindowAbout();

    bool showProgramsWindow();
    void closeProgramsWindow();

    virtual bool showProgramEditForm(const QString &appPath);
    void openProgramEditForm(const QString &appPath, qint64 appId, FormWindow *parentForm);

    void showProgramAlertWindow(bool activate = true);
    void closeProgramAlertWindow();

    void showOptionsWindow();
    void closeOptionsWindow();
    void reloadOptionsWindow(const QString &reason);

    void showRulesWindow();
    void closeRulesWindow();

    void showStatisticsWindow();
    void closeStatisticsWindow();

    void showOptionsWindowTab(int index);
    void showAppGroupsWindow();

    void showServicesWindow();
    void closeServicesWindow();

    void showZonesWindow();
    void closeZonesWindow();

    void showGroupsWindow();
    void closeGroupsWindow();

    void showSpeedLimitsWindow();
    void closeSpeedLimitsWindow();

    void showGraphWindow();
    void closeGraphWindow();

    void showWindowByCode(WindowCode code);
    void closeWindowByCode(WindowCode code);
    void switchWindowByCode(WindowCode code);

    void closeAllWindows();

    void quit();

    void processRestartRequired(const QString &info = {});

    bool checkWindowPassword(WindowCode code);
    virtual bool checkPassword(WindowCode code = WindowNone);

    void resetCheckedPassword();

    virtual void showErrorBox(
            const QString &text, const QString &title = QString(), QWidget *parent = nullptr);
    virtual void showInfoBox(
            const QString &text, const QString &title = QString(), QWidget *parent = nullptr);
    void showConfirmBox(const std::function<void()> &onConfirmed, const QString &text,
            const QString &title = QString(), QWidget *parent = nullptr);
    void showQuestionBox(const std::function<void(bool confirmed)> &onFinished, const QString &text,
            const QString &title = QString(), QWidget *parent = nullptr);

    static void showMessageBox(QMessageBox::Icon icon, const QString &text,
            const QString &title = QString(), QWidget *parent = nullptr);

    static bool showPasswordDialog(QString &password, int &unlockType);

    static bool activateModalWidget();

    static void updateTheme(const IniUser &ini);
    static void updateStyle(const IniUser &ini);

private:
    void setupAppPalette();
    static void refreshAppPalette();

    void setupMainWindow();
    void closeMainWindow();

    void setupHomeWindow();
    void setupProgramsWindow();
    void setupProgramAlertWindow();
    void setupOptionsWindow();
    void setupRulesWindow();
    void setupServicesWindow();
    void setupZonesWindow();
    void setupGroupsWindow();
    void setupSpeedLimitsWindow();
    void setupGraphWindow();
    void setupStatisticsWindow();

    void setupConfManager();
    void setupByIniUser(const IniUser &ini);

    void updateTrayIconVisibility(const IniUser &ini);
    void updateGraphWindowVisibility(const IniUser &ini);

    void closeAll();
    void quitApp();

    bool checkPasswordDialog(WindowCode code, FortSettings *settings);

    void onTrayMessageClicked();

    void showWindow(FormWindow *w, bool activate = true);
    bool closeWindow(FormWindow *w);

    void windowOpened(WindowCode code);
    bool isAnyWindowOpen(quint32 codes) const;

    void windowUnlocked(WindowCode code);
    bool isAnyWindowUnlocked() const;

    void windowClosed(WindowCode code);

private:
    bool m_isAppQuitting = false;

    quint32 m_openedWindows = 0;
    quint32 m_unlockedWindows = 0;

    TrayMessageType m_lastTrayMessageType = TrayMessageOptions;

    TaskbarButton m_taskbarButton;

    TrayIcon *m_trayIcon = nullptr;

    MainWindow *m_mainWindow = nullptr;
    HomeWindow *m_homeWindow = nullptr;
    ProgramsWindow *m_progWindow = nullptr;
    ProgramAlertWindow *m_progAlertWindow = nullptr;
    OptionsWindow *m_optWindow = nullptr;
    RulesWindow *m_rulesWindow = nullptr;
    StatisticsWindow *m_statWindow = nullptr;
    ServicesWindow *m_servicesWindow = nullptr;
    ZonesWindow *m_zonesWindow = nullptr;
    GroupsWindow *m_groupsWindow = nullptr;
    SpeedLimitsWindow *m_speedLimitsWindow = nullptr;
    GraphWindow *m_graphWindow = nullptr;
};

#endif // WINDOWMANAGER_H
