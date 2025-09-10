#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QMessageBox>
#include <QObject>

#include <form/form_types.h>
#include <form/formpointer.h>
#include <form/tray/trayicon_types.h>
#include <util/ioc/iocservice.h>
#include <util/taskbarbutton.h>

class FormWindow;
class FortSettings;
class GraphWindow;
class HomeWindow;
class IniUser;
class MainWindow;
class OptionsWindow;
class ProgramAlertWindow;
class ProgramsWindow;
class RulesWindow;
class ServicesWindow;
class StatisticsWindow;
class TrayIcon;
class WidgetWindow;
class ZonesWindow;

class WindowManager : public QObject, public IocService
{
    Q_OBJECT

    friend class FormPointer;

public:
    explicit WindowManager(QObject *parent = nullptr);

    bool isAppQuitting() const { return m_isAppQuitting; }

    TaskbarButton &taskbarButton() { return m_taskbarButton; }

    MainWindow *mainWindow() const { return m_mainWindow; }
    TrayIcon *trayIcon() const { return m_trayIcon; }

    static bool hasForm(WindowCode code);
    FormPointer &formByCode(WindowCode code) const;
    FormWindow *windowByCode(WindowCode code) const;

    HomeWindow *homeWindow() const;
    ProgramsWindow *progWindow() const;
    ProgramAlertWindow *progAlertWindow() const;
    RulesWindow *rulesWindow() const;
    OptionsWindow *optWindow() const;
    StatisticsWindow *statWindow() const;
    ServicesWindow *servicesWindow() const;
    ZonesWindow *zonesWindow() const;
    GraphWindow *graphWindow() const;

    void setUp() override;
    void tearDown() override;

    void initialize();

    bool isWindowOpen(WindowCode code) const { return isAnyWindowOpen(code); }

    static QFont defaultFont();

signals:
    void windowVisibilityChanged(WindowCode code, bool isVisible);

public slots:
    void showTrayIcon();
    void closeTrayIcon();

    void showTrayMessage(const QString &message, tray::MessageType type = tray::MessageOptions);

    void showSplashScreen();

    bool showHomeWindow() { return showWindowByCode(WindowHome); }

    virtual bool exposeHomeWindow();
    void showHomeWindowAbout();

    bool showProgramsWindow() { return showWindowByCode(WindowPrograms); }
    bool closeProgramsWindow() { return closeWindowByCode(WindowPrograms); }

    virtual bool showProgramEditForm(const QString &appPath);
    void openProgramEditForm(const QString &appPath, qint64 appId, FormWindow *parentForm);

    bool showProgramAlertWindow(bool activate = true);
    bool closeProgramAlertWindow() { return closeWindowByCode(WindowProgramAlert); }

    bool showServicesWindow() { return showWindowByCode(WindowServices); }

    bool showOptionsWindow() { return showWindowByCode(WindowOptions); }
    bool closeOptionsWindow() { return closeWindowByCode(WindowOptions); }

    bool reloadOptionsWindow(const QString &reason);

    bool showOptionsWindowTab(int index);
    void showAppGroupsWindow();

    bool showRulesWindow() { return showWindowByCode(WindowRules); }

    bool showStatisticsWindow() { return showWindowByCode(WindowStatistics); }

    bool showZonesWindow() { return showWindowByCode(WindowZones); }

    bool showGraphWindow() { return showWindowByCode(WindowGraph, /*activate=*/false); }

    bool showWindowByCode(WindowCode code, bool activate = true);
    bool closeWindowByCode(WindowCode code);
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

    void setupConfManager();
    void setupByIniUser(const IniUser &ini);

    void updateTrayIconVisibility(const IniUser &ini);
    void updateGraphWindowVisibility(const IniUser &ini);

    void closeAll();
    void quitApp();

    bool checkPasswordDialog(WindowCode code, FortSettings *settings);

    void windowOpened(WindowCode code);
    bool isAnyWindowOpen(quint32 codes) const;

    void windowUnlocked(WindowCode code);
    bool isAnyWindowUnlocked() const;

    void windowClosed(WindowCode code);

    using WindowFunc = bool (WindowManager::*)(void);
    using WindowFuncArray = std::array<WindowFunc, WindowCount>;

    bool doWindowFunc(const WindowFuncArray &funcs, WindowCode code);

private:
    bool m_isAppQuitting = false;

    quint32 m_openedWindows = 0;
    quint32 m_unlockedWindows = 0;

    TaskbarButton m_taskbarButton;

    MainWindow *m_mainWindow = nullptr;
    TrayIcon *m_trayIcon = nullptr;

    mutable std::array<FormPointer, WindowCount> m_forms = {
        FormPointer(WindowHome),
        FormPointer(WindowPrograms),
        FormPointer(WindowProgramAlert),
        FormPointer(WindowServices),
        FormPointer(WindowOptions),
        FormPointer(WindowRules),
        FormPointer(WindowStatistics),
        FormPointer(WindowZones),
        FormPointer(WindowGraph),
    };
};

#endif // WINDOWMANAGER_H
