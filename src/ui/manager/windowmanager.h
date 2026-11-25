#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QMessageBox>
#include <QObject>
#include <QVariant>

#include <functional>

#include <form/form_types.h>
#include <form/formpointer.h>
#include <form/tray/trayicon_types.h>
#include <user/inigraph.h>
#include <util/ioc/iocservice.h>
#include <util/taskbarbutton.h>

class Conn;
class ConnectionsWindow;
class FilterSimWindow;
class FormWindow;
class FortSettings;
class GraphWindow;
class GroupsWindow;
class IniUser;
class MainWindow;
class NativeWindow;
class OptionsWindow;
class ProgramAlertWindow;
class ProgramEditDialog;
class ProgramsWindow;
class RulesWindow;
class ServicesWindow;
class SpeedLimitsWindow;
class TimePeriodsWindow;
class TrafficWindow;
class TrayIcon;
class WidgetWindow;
class ZonesWindow;

struct MessageBoxArg;

class WindowManager : public QObject, public IocService
{
    Q_OBJECT

    friend class FormPointer;

public:
    explicit WindowManager(QObject *parent = nullptr);

    bool isAppQuitting() const { return m_isAppQuitting; }

    TaskbarButton &taskbarButton() { return m_taskbarButton; }

    NativeWindow *nativeWindow() const { return m_nativeWindow; }
    TrayIcon *trayIcon() const { return m_trayIcon; }

    static bool hasForm(WindowCode code);
    FormPointer &formByCode(WindowCode code) const;
    FormWindow *windowByCode(WindowCode code) const;

    FormPointer &graphFormById(int graphId) const;

    MainWindow *mainWindow() const;
    ProgramsWindow *progWindow() const;
    ProgramAlertWindow *progAlertWindow() const;
    RulesWindow *rulesWindow() const;
    OptionsWindow *optWindow() const;
    TrafficWindow *trafWindow() const;
    ConnectionsWindow *connWindow() const;
    ServicesWindow *servicesWindow() const;
    ZonesWindow *zonesWindow() const;
    GroupsWindow *groupsWindow() const;
    SpeedLimitsWindow *speedLimitsWindow() const;
    TimePeriodsWindow *timePeriodsWindow() const;
    FilterSimWindow *filterSimWindow() const;
    GraphWindow *graphWindow(int graphId = 0) const;

    void setUp() override;
    void tearDown() override;

    virtual void initialize();

    bool isWindowOpen(WindowCode code) const { return isAnyWindowOpen(code); }

    // Shows the docked window in the Main window without the password: it is disabled there
    bool restoreDockedWindowByCode(WindowCode code);

    // Not a slot: Qt 6.1's moc needs operator<() of std::optional<QPoint>
    bool dockWindowByCode(WindowCode code);
    bool undockWindowByCode(WindowCode code, const std::optional<QPoint> &pos = std::nullopt);
    void closeDockedWindows();

    static QFont defaultFont();
    static QSize listIconSize();

signals:
    void windowVisibilityChanged(WindowCode code, bool isVisible);
    void graphWindowVisibilityChanged(int graphId, bool isVisible);

    void requestQuit();

    /* The windows without a code, e.g. the Program Connections, close on it */
    void aboutToCloseAllWindows();

public slots:
    void showTrayIcon();
    void closeTrayIcon();

    void showTrayMessage(const QString &message, tray::MessageType type = tray::MessageOptions);

    void showSplashScreen();

    bool showMainWindow(bool expose = false)
    {
        return showWindowByCode(WindowMain, /*activate=*/true, expose);
    }

    virtual bool exposeMainWindow();

    bool showProgramsWindow() { return showWindowByCode(WindowPrograms); }
    bool closeProgramsWindow() { return closeWindowByCode(WindowPrograms); }

    virtual bool showProgramEditForm(const QString &appPath);
    ProgramEditDialog *openProgramEditForm(
            const QString &appPath, qint64 appId = 0, FormWindow *parentForm = nullptr);

    bool showProgramAlertWindow(bool activate = true);

    bool showServicesWindow() { return showWindowByCode(WindowServices); }

    bool showServicesWindowService(const QString &serviceName);

    bool showOptionsWindow() { return showWindowByCode(WindowOptions); }
    bool closeOptionsWindow() { return closeWindowByCode(WindowOptions); }

    bool showOptionsWindowTab(int index);

    bool showRulesWindow() { return showWindowByCode(WindowRules); }

    bool showTrafficWindow() { return showWindowByCode(WindowTraffic); }

    bool showConnectionsWindow() { return showWindowByCode(WindowConnections); }

    bool showZonesWindow() { return showWindowByCode(WindowZones); }

    bool showGroupsWindow() { return showWindowByCode(WindowGroups); }

    bool showSpeedLimitsWindow() { return showWindowByCode(WindowSpeedLimits); }

    bool showTimePeriodsWindow() { return showWindowByCode(WindowTimePeriods); }

    bool showFilterSimWindow() { return showWindowByCode(WindowFilterSim); }

    bool showFilterSimWindowConn(const Conn &conn);

    bool showGraphWindow(int graphId = 0);
    bool closeGraphWindow(int graphId = 0);
    void deleteGraphWindow(int graphId);

    bool isGraphWindowShown(int graphId) const;

    bool setGraphWindowVisible(const QVariant &visibleOrSwitch = {})
    {
        return setWindowVisibleByCode(WindowGraph, visibleOrSwitch, /*activate=*/false);
    }

    bool showInstallationWindow() { return showWindowByCode(WindowInstallation); }

    bool showUpdatesWindow() { return showWindowByCode(WindowUpdates); }

    void showAboutBox();

    virtual bool setWindowVisibleByCode(
            WindowCode code, const QVariant &visibleOrSwitch = {}, bool activate = true);

    bool showWindowByCode(WindowCode code, bool activate = true, bool expose = false);
    bool closeWindowByCode(WindowCode code);
    void switchWindowByCode(WindowCode code);

    void saveAllWindowStates();
    void closeAllWindows(bool isAppQuitting = false);

    void quit();

    void processRestartRequired(const QString &info = {});

    bool checkWindowPassword(WindowCode code);
    virtual bool checkPassword(WindowCode code = WindowNone);

    void resetCheckedPassword();

    virtual void showErrorBox(
            const QString &text, const QString &title = QString(), QWidget *parent = nullptr);
    virtual void showInfoBox(
            const QString &text, const QString &title = QString(), QWidget *parent = nullptr);
    // The callback is called by the context: the box is closed with it
    void showConfirmBox(QObject *context, const std::function<void()> &onConfirmed,
            const QString &text, const QString &title = QString());
    void showQuestionBox(QObject *context, const std::function<void(bool confirmed)> &onFinished,
            const QString &text, const QString &title = QString());
    void showChoiceBox(QObject *context, const std::function<void(int button)> &onFinished,
            const MessageBoxArg &ba);

    static void showMessageBox(QMessageBox::Icon icon, const QString &text,
            const QString &title = QString(), QWidget *parent = nullptr);

    static bool showPasswordDialog(QString &password, int &unlockType);

    static bool activateModalWidget();

    static void updateTheme(const IniUser &ini);
    static void updateStyle(const IniUser &ini);
    static void updateStyleSheet(const IniUser &ini);
    static void updateFont(const IniUser &ini);
    static void updateListIconSize(const IniUser &ini);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupAppPalette();
    static void refreshAppPalette();

    void setupNativeWindow();
    void closeNativeWindow();

    void setupConfManager();
    void setupByIniUser();

    void updateTrayIconVisibility(const IniUser &ini);
    void showGraphWindows();
    void updateGraphWindowsVisibility();
    void updateGraphWindowVisibility(int graphId);

    void walkForms(const std::function<void(FormPointer &form)> &func) const;

    void closeAll();
    void quitApp();

    bool checkPasswordDialog(WindowCode code, FortSettings *settings);

    void onPasswordCheckedChanged();

    void windowOpened(WindowCode code, int id = 0);
    bool isAnyWindowOpen(quint32 codes) const;

    void windowUnlocked(WindowCode code);
    bool isAnyWindowUnlocked() const;

    void windowClosed(WindowCode code, int id = 0);

private:
    bool m_isAppQuitting = false;

    quint32 m_openedWindows = 0;
    quint32 m_unlockedWindows = 0;

    TaskbarButton m_taskbarButton;

    NativeWindow *m_nativeWindow = nullptr;
    TrayIcon *m_trayIcon = nullptr;

    mutable std::array<FormPointer, WindowCount> m_forms = {
        FormPointer(WindowMain),
        FormPointer(WindowPrograms),
        FormPointer(WindowProgramAlert),
        FormPointer(WindowServices),
        FormPointer(WindowOptions),
        FormPointer(WindowRules),
        FormPointer(WindowTraffic),
        FormPointer(WindowActivity),
        FormPointer(WindowConnections),
        FormPointer(WindowZones),
        FormPointer(WindowGroups),
        FormPointer(WindowSpeedLimits),
        FormPointer(WindowTimePeriods),
        FormPointer(WindowFilterSim),
        FormPointer(WindowGraph),
        FormPointer(WindowInstallation),
        FormPointer(WindowUpdates),
        FormPointer(WindowGraphs),
    };

    // The graphs' windows but the main one (of m_forms)
    mutable std::array<FormPointer, IniGraph::maxCount - 1> m_graphForms = {
        FormPointer(WindowGraph, 1),
        FormPointer(WindowGraph, 2),
        FormPointer(WindowGraph, 3),
        FormPointer(WindowGraph, 4),
        FormPointer(WindowGraph, 5),
        FormPointer(WindowGraph, 6),
        FormPointer(WindowGraph, 7),
    };
};

#endif // WINDOWMANAGER_H
