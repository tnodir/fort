#include "windowmanager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QProcess>
#include <QStyle>
#include <QStyleHints>

#include <conf/confmanager.h>
#include <form/conn/connectionswindow.h>
#include <form/controls/mainwindow.h>
#include <form/dialog/dialogutil.h>
#include <form/dialog/passworddialog.h>
#include <form/dialog/splashscreen.h>
#include <form/filtersim/filtersimwindow.h>
#include <form/graph/graphwindow.h>
#include <form/group/groupswindow.h>
#include <form/home/homewindow.h>
#include <form/opt/optionswindow.h>
#include <form/prog/programalertwindow.h>
#include <form/prog/programswindow.h>
#include <form/rule/ruleswindow.h>
#include <form/speedlimit/speedlimitswindow.h>
#include <form/svc/serviceswindow.h>
#include <form/timeperiod/timeperiodswindow.h>
#include <form/traf/trafficwindow.h>
#include <form/tray/trayicon.h>
#include <form/zone/zoneswindow.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <stat/statmanager.h>
#include <user/usersettings.h>
#include <util/bitutil.h>
#include <util/guiutil.h>
#include <util/osutil.h>

#include "nativeeventfilter.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("manager.window");

bool g_isFusionStyle = false;

}

WindowManager::WindowManager(QObject *parent) : QObject(parent) { }

bool WindowManager::hasForm(WindowCode code)
{
    constexpr quint32 windowForms =
            (WindowHome | WindowPrograms | WindowProgramAlert | WindowServices | WindowOptions
                    | WindowRules | WindowTraffic | WindowConnections | WindowZones | WindowGroups
                    | WindowSpeedLimits | WindowTimePeriods | WindowFilterSim | WindowGraph);

    return (code & windowForms) != 0;
}

FormPointer &WindowManager::formByCode(WindowCode code) const
{
    Q_ASSERT(hasForm(code));

    int index = BitUtil::bitScanForward(code);

    if (Q_UNLIKELY(index < 0 || index >= WindowCount)) {
        Q_UNREACHABLE();
        index = 0;
    }

    return m_forms[index];
}

FormWindow *WindowManager::windowByCode(WindowCode code) const
{
    const auto &form = formByCode(code);
    return form.window();
}

HomeWindow *WindowManager::homeWindow() const
{
    return static_cast<HomeWindow *>(windowByCode(WindowHome));
}

ProgramsWindow *WindowManager::progWindow() const
{
    return static_cast<ProgramsWindow *>(windowByCode(WindowPrograms));
}

ProgramAlertWindow *WindowManager::progAlertWindow() const
{
    return static_cast<ProgramAlertWindow *>(windowByCode(WindowProgramAlert));
}

RulesWindow *WindowManager::rulesWindow() const
{
    return static_cast<RulesWindow *>(windowByCode(WindowRules));
}

OptionsWindow *WindowManager::optWindow() const
{
    return static_cast<OptionsWindow *>(windowByCode(WindowOptions));
}

TrafficWindow *WindowManager::trafWindow() const
{
    return static_cast<TrafficWindow *>(windowByCode(WindowTraffic));
}

ConnectionsWindow *WindowManager::connWindow() const
{
    return static_cast<ConnectionsWindow *>(windowByCode(WindowConnections));
}

ServicesWindow *WindowManager::servicesWindow() const
{
    return static_cast<ServicesWindow *>(windowByCode(WindowServices));
}

ZonesWindow *WindowManager::zonesWindow() const
{
    return static_cast<ZonesWindow *>(windowByCode(WindowZones));
}

GroupsWindow *WindowManager::groupsWindow() const
{
    return static_cast<GroupsWindow *>(windowByCode(WindowGroups));
}

SpeedLimitsWindow *WindowManager::speedLimitsWindow() const
{
    return static_cast<SpeedLimitsWindow *>(windowByCode(WindowSpeedLimits));
}

TimePeriodsWindow *WindowManager::timePeriodsWindow() const
{
    return static_cast<TimePeriodsWindow *>(windowByCode(WindowTimePeriods));
}

FilterSimWindow *WindowManager::filterSimWindow() const
{
    return static_cast<FilterSimWindow *>(windowByCode(WindowFilterSim));
}

GraphWindow *WindowManager::graphWindow() const
{
    return static_cast<GraphWindow *>(windowByCode(WindowGraph));
}

void WindowManager::setUp()
{
    setupAppPalette();

    setupMainWindow();
    setupConfManager();

    qApp->installEventFilter(this);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &WindowManager::quitApp);
}

void WindowManager::tearDown()
{
    closeAll();
}

void WindowManager::initialize()
{
    if (!m_trayIcon) {
        m_trayIcon = new TrayIcon(this);
    }

    const auto &ini = Fort::iniUser();

    if (ini.splashWindowVisible() && !settings()->noSplash()) {
        showSplashScreen();
    }

    if (ini.graphWindowVisible()) {
        showGraphWindow();
    }

    setupByIniUser();
}

QFont WindowManager::defaultFont()
{
    static QFont g_font(
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
            "Tahoma",
#else
            QStringList { "Tahoma" },
#endif
            9);

    return g_font;
}

void WindowManager::setupAppPalette()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &WindowManager::refreshAppPalette);
#endif
}

void WindowManager::refreshAppPalette()
{
    const QPalette palette =
            g_isFusionStyle ? QApplication::style()->standardPalette() : QPalette();

    QApplication::setPalette(palette);
}

void WindowManager::setupMainWindow()
{
    m_mainWindow = new MainWindow();

    // Font
    m_mainWindow->setFont(defaultFont());

    // Register Native events
    auto nativeEventFilter = Fort::dependency<NativeEventFilter>();

    nativeEventFilter->registerSessionNotification(mainWindow()->winId());

    connect(nativeEventFilter, &NativeEventFilter::sessionLocked, this,
            [&] { settings()->resetCheckedPassword(FortSettings::UnlockSession); });
}

void WindowManager::closeMainWindow()
{
    if (!mainWindow())
        return;

    // Unregister Native events
    auto nativeEventFilter = Fort::nativeEventFilter();

    nativeEventFilter->unregisterHotKeys();
    nativeEventFilter->unregisterSessionNotification(mainWindow()->winId());

    // Delete later
    m_mainWindow->deleteLater();
    m_mainWindow = nullptr;
}

void WindowManager::setupConfManager()
{
    auto confManager = Fort::dependency<ConfManager>();

    connect(confManager, &ConfManager::iniUserChanged, this, &WindowManager::setupByIniUser);
}

void WindowManager::setupByIniUser()
{
    const auto &ini = Fort::iniUser();

    updateTheme(ini);
    updateStyle(ini);
    updateTrayIconVisibility(ini);
    updateGraphWindowVisibility(ini);
}

void WindowManager::updateTheme(const IniUser &ini)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    const auto colorScheme = Qt::ColorScheme(IniUser::colorSchemeByName(ini.theme()));

    QApplication::styleHints()->setColorScheme(colorScheme);
#else
    Q_UNUSED(ini);
#endif
}

void WindowManager::updateStyle(const IniUser &ini)
{
    const QString style = ini.style();

    g_isFusionStyle = (style == ini.styleDefault());

    QApplication::setStyle(style);

    refreshAppPalette();
}

void WindowManager::updateTrayIconVisibility(const IniUser &ini)
{
    if (ini.trayShowIcon()) {
        showTrayIcon();
    } else {
        closeTrayIcon();

        if (ini.homeWindowAutoShowWindow() && !homeWindow()) {
            showHomeWindow();
        }
    }
}

void WindowManager::updateGraphWindowVisibility(const IniUser &ini)
{
    auto &form = formByCode(WindowGraph);
    const auto w = form.window();

    if (w && w->isVisible())
        return;

    if (ini.graphWindowHideOnClose()) {
        if (!w) {
            form.initialize();
        }
    } else {
        form.close();
    }
}

void WindowManager::showTrayIcon()
{
    m_trayIcon->show();
}

void WindowManager::closeTrayIcon()
{
    m_trayIcon->hide();
}

void WindowManager::showTrayMessage(const QString &message, tray::MessageType type)
{
    if (m_trayIcon) {
        m_trayIcon->showTrayMessage(message, type);
    }
}

void WindowManager::showSplashScreen()
{
    auto splash = new SplashScreen(); // auto-deleted on close
    splash->showFading();
}

bool WindowManager::exposeHomeWindow()
{
    showHomeWindow();
    homeWindow()->exposeWindow();
    return true;
}

void WindowManager::showHomeWindowAbout()
{
    showHomeWindow();
    homeWindow()->selectAboutTab();
}

bool WindowManager::showProgramEditForm(const QString &appPath)
{
    openProgramEditForm(appPath);
    return true;
}

void WindowManager::openProgramEditForm(
        const QString &appPath, qint64 appId, FormWindow *parentForm)
{
    ProgramsWindow::openProgramByPath(appPath, appId, parentForm);
}

bool WindowManager::showProgramAlertWindow(bool activate)
{
    if (!showWindowByCode(WindowProgramAlert, /*activate=*/false))
        return false;

    auto w = progAlertWindow();

    if (w->isNew()) {
        showProgramsWindow(); // reuse checked password
        closeProgramAlertWindow();
    } else {
        if (activate || w->isAutoActive()) {
            w->activateWindow();
        }
    }

    return true;
}

bool WindowManager::reloadOptionsWindow(const QString &reason)
{
    if (!optWindow())
        return false;

    // Unsaved changes are lost
    closeOptionsWindow();

    // Show after new conf initialization
    QMetaObject::invokeMethod(this, &WindowManager::showOptionsWindow, Qt::QueuedConnection);

    showTrayMessage(reason);

    return true;
}

bool WindowManager::showOptionsWindowTab(int index)
{
    if (!showOptionsWindow())
        return false;

    optWindow()->selectTab(index);

    return true;
}

bool WindowManager::setWindowVisibleByCode(
        WindowCode code, const QVariant &visibleOrSwitch, bool activate)
{
    const bool visible =
            !visibleOrSwitch.isNull() ? visibleOrSwitch.toBool() : !isAnyWindowOpen(code);

    if (visible) {
        return showWindowByCode(code, activate);
    }

    closeWindowByCode(code);
    return true;
}

bool WindowManager::showWindowByCode(WindowCode code, bool activate)
{
    auto &form = formByCode(code);
    return form.show(activate);
}

bool WindowManager::closeWindowByCode(WindowCode code)
{
    auto &form = formByCode(code);
    return form.close();
}

void WindowManager::switchWindowByCode(WindowCode code)
{
    if (isAnyWindowOpen(code)) {
        closeWindowByCode(code);
    } else {
        showWindowByCode(code);
    }
}

void WindowManager::saveAllWindowStates()
{
    for (int i = 0; i < WindowCount; ++i) {
        const auto w = m_forms[i].window();
        if (!w || !w->isVisible())
            continue;

        w->saveWindowState(/*wasVisible=*/true);
    }
}

void WindowManager::closeAllWindows(bool isAppQuitting)
{
    for (int i = 0; i < WindowCount; ++i) {
        auto &form = m_forms[i];
        form.close(isAppQuitting);
    }
}

void WindowManager::closeAll()
{
    closeAllWindows();

    closeTrayIcon();
    closeMainWindow();
}

void WindowManager::quitApp()
{
    if (m_isAppQuitting)
        return;

    m_isAppQuitting = true;

    closeAll();
}

bool WindowManager::eventFilter(QObject *watched, QEvent *event)
{
    // The Quit event closes the windows as by the user: close them before as on quit
    if (event->type() == QEvent::Quit) {
        quitApp();
    }

    return QObject::eventFilter(watched, event);
}

void WindowManager::quit()
{
    quitApp();

    OsUtil::quit("user request");
}

void WindowManager::processRestartRequired(const QString &info)
{
    const QString title = tr("Restart Required");
    const QString text = tr("Restart Now?") + "\n\n" + info;

    showConfirmBox([&] { OsUtil::restart(); }, text, title);
}

bool WindowManager::checkWindowPassword(WindowCode code)
{
    if ((code & WindowPasswordProtected) == 0)
        return true;

    return checkPassword();
}

bool WindowManager::checkPassword(WindowCode code)
{
    if (isAnyWindowOpen(WindowPasswordDialog)) {
        activateModalWidget();
        return false;
    }

    if (isAnyWindowUnlocked())
        return true;

    const auto settings = Fort::settings();

    if (!settings->isPasswordRequired() || settings->passwordTemporaryChecked())
        return true;

    return checkPasswordDialog(code, settings);
}

void WindowManager::resetCheckedPassword()
{
    m_unlockedWindows = 0;

    settings()->resetCheckedPassword();
}

void WindowManager::showErrorBox(const QString &text, const QString &title, QWidget *parent)
{
    showMessageBox(QMessageBox::Warning, text, title, parent);
}

void WindowManager::showInfoBox(const QString &text, const QString &title, QWidget *parent)
{
    showMessageBox(QMessageBox::Information, text, title, parent);
}

void WindowManager::showConfirmBox(const std::function<void()> &onConfirmed, const QString &text,
        const QString &title, QWidget *parent)
{
    showQuestionBox(
            [=](bool confirmed) {
                if (confirmed) {
                    onConfirmed();
                }
            },
            text, title, parent);
}

void WindowManager::showQuestionBox(const std::function<void(bool confirmed)> &onFinished,
        const QString &text, const QString &title, QWidget *parent)
{
    showChoiceBox(
            [=](int button) {
                const bool confirmed = (button == QMessageBox::Yes);
                onFinished(confirmed);
            },
            { .icon = QMessageBox::Question,
                    .buttons = QMessageBox::Yes | QMessageBox::No,
                    .text = text,
                    .title = title },
            parent);
}

void WindowManager::showChoiceBox(
        const std::function<void(int button)> &onFinished, const MessageBoxArg &ba, QWidget *parent)
{
    auto box = DialogUtil::createMessageBox(ba, parent);

    connect(box, &QMessageBox::finished, this, onFinished, Qt::QueuedConnection);

    DialogUtil::showDialog(box);
}

void WindowManager::showMessageBox(
        QMessageBox::Icon icon, const QString &text, const QString &title, QWidget *parent)
{
    auto box = DialogUtil::createMessageBox(
            {
                    .icon = icon,
                    .buttons = QMessageBox::Ok,
                    .text = text,
                    .title = title,
            },
            parent);

    DialogUtil::showDialog(box);
}

bool WindowManager::showPasswordDialog(QString &password, int &unlockType)
{
    auto box = new PasswordDialog();

    DialogUtil::setupModalDialog(box);
    DialogUtil::showDialog(box);

    const bool accepted = (box->exec() == QDialog::Accepted);

    password = box->password();
    unlockType = box->unlockType();

    box->deleteLater();

    return accepted && !password.isEmpty();
}

bool WindowManager::checkPasswordDialog(WindowCode code, FortSettings *settings)
{
    QString password;
    int unlockType = FortSettings::UnlockDisabled;

    windowOpened(WindowPasswordDialog);

    const bool ok = showPasswordDialog(password, unlockType);

    windowClosed(WindowPasswordDialog);

    if (!(ok && confManager()->checkPassword(password)))
        return false;

    windowUnlocked(code);

    settings->setPasswordChecked(true, FortSettings::UnlockType(unlockType));

    return true;
}

void WindowManager::windowOpened(WindowCode code)
{
    m_openedWindows |= code;
    m_unlockedWindows |= (code & WindowPasswordProtected);

    emit windowVisibilityChanged(code, /*isVisible=*/true);
}

bool WindowManager::isAnyWindowOpen(quint32 codes) const
{
    return (m_openedWindows & codes) != 0;
}

void WindowManager::windowUnlocked(WindowCode code)
{
    if (code == WindowNone)
        return;

    m_unlockedWindows |= code;
}

bool WindowManager::isAnyWindowUnlocked() const
{
    return m_unlockedWindows != 0;
}

void WindowManager::windowClosed(WindowCode code)
{
    m_openedWindows &= ~code;
    m_unlockedWindows &= ~code;

    emit windowVisibilityChanged(code, /*isVisible=*/false);

    if (!isAnyWindowUnlocked()) {
        settings()->resetCheckedPassword(FortSettings::UnlockWindow);
    }
}

bool WindowManager::doWindowFunc(const WindowFuncArray &funcs, WindowCode code)
{
    const int index = BitUtil::bitScanForward(code);

    if (Q_UNLIKELY(index < 0 || code >= WindowCount))
        return false;

    return std::invoke(funcs[index], this);
}

bool WindowManager::activateModalWidget()
{
    auto w = QApplication::activeModalWidget();
    if (w && w->windowModality() == Qt::ApplicationModal) {
        WidgetWindow::showWidget(w);
        return true;
    }
    return false;
}
