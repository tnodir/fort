#include "windowmanager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QProcess>
#include <QStyle>
#include <QStyleHints>

#include <conf/confmanager.h>
#include <form/controls/mainwindow.h>
#include <form/dialog/dialogutil.h>
#include <form/dialog/passworddialog.h>
#include <form/dialog/splashscreen.h>
#include <form/graph/graphwindow.h>
#include <form/group/groupswindow.h>
#include <form/home/homewindow.h>
#include <form/opt/optionswindow.h>
#include <form/prog/programalertwindow.h>
#include <form/prog/programswindow.h>
#include <form/rule/ruleswindow.h>
#include <form/stat/statisticswindow.h>
#include <form/svc/serviceswindow.h>
#include <form/tray/trayicon.h>
#include <form/zone/zoneswindow.h>
#include <fortsettings.h>
#include <stat/statmanager.h>
#include <user/usersettings.h>
#include <util/bitutil.h>
#include <util/guiutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "nativeeventfilter.h"

namespace {

const QLoggingCategory LC("manager.window");

bool g_isFusionStyle = false;

}

WindowManager::WindowManager(QObject *parent) : QObject(parent) { }

void WindowManager::setUp()
{
    setupAppPalette();

    setupMainWindow();
    setupConfManager();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &WindowManager::quitApp);
}

void WindowManager::tearDown()
{
    closeAll();
}

void WindowManager::initialize()
{
    const IniUser &ini = IoC<UserSettings>()->iniUser();

    setupTrayIcon();

    if (ini.splashWindowVisible() && !IoC<FortSettings>()->noSplash()) {
        showSplashScreen();
    }

    if (ini.graphWindowVisible()) {
        showGraphWindow();
    }

    setupByIniUser(ini);
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
    auto nativeEventFilter = IoCDependency<NativeEventFilter>();

    nativeEventFilter->registerSessionNotification(mainWindow()->winId());

    connect(nativeEventFilter, &NativeEventFilter::sessionLocked, this,
            [&] { IoC<FortSettings>()->resetCheckedPassword(FortSettings::UnlockSession); });
}

void WindowManager::closeMainWindow()
{
    if (!mainWindow())
        return;

    // Unregister Native events
    auto nativeEventFilter = IoC<NativeEventFilter>();

    nativeEventFilter->unregisterHotKeys();
    nativeEventFilter->unregisterSessionNotification(mainWindow()->winId());

    // Delete later
    m_mainWindow->deleteLater();
    m_mainWindow = nullptr;
}

void WindowManager::setupHomeWindow()
{
    m_homeWindow = new HomeWindow();

    connect(m_homeWindow, &HomeWindow::aboutToClose, this, [&](QEvent *event) {
        const IniUser &ini = IoC<UserSettings>()->iniUser();

        if (ini.homeWindowAutoShowWindow()) {
            quitHomeWindow(event);
        }
    });
}

void WindowManager::setupProgramsWindow()
{
    m_progWindow = new ProgramsWindow();

    connect(m_progWindow, &ProgramsWindow::aboutToClose, this, &WindowManager::closeProgramsWindow);
}

void WindowManager::setupProgramAlertWindow()
{
    m_progAlertWindow = new ProgramAlertWindow();

    connect(m_progAlertWindow, &ProgramAlertWindow::aboutToClose, this,
            &WindowManager::closeProgramAlertWindow);
}

void WindowManager::setupOptionsWindow()
{
    m_optWindow = new OptionsWindow();

    connect(m_optWindow, &OptionsWindow::aboutToClose, this, &WindowManager::closeOptionsWindow);
}

void WindowManager::setupRulesWindow()
{
    m_rulesWindow = new RulesWindow();

    connect(m_rulesWindow, &RulesWindow::aboutToClose, this, &WindowManager::closeRulesWindow);
}

void WindowManager::setupServicesWindow()
{
    m_servicesWindow = new ServicesWindow();

    connect(m_servicesWindow, &ServicesWindow::aboutToClose, this,
            &WindowManager::closeServicesWindow);
}

void WindowManager::setupZonesWindow()
{
    m_zonesWindow = new ZonesWindow();

    connect(m_zonesWindow, &ZonesWindow::aboutToClose, this, &WindowManager::closeZonesWindow);
}

void WindowManager::setupGroupsWindow()
{
    m_groupsWindow = new GroupsWindow();

    connect(m_groupsWindow, &GroupsWindow::aboutToClose, this, &WindowManager::closeGroupsWindow);
}

void WindowManager::setupSpeedLimitsWindow()
{
#if 0 // TODO
    m_speedLimitsWindow = new SpeedLimitsWindow();

    connect(m_speedLimitsWindow, &SpeedLimitsWindow::aboutToClose, this,
            &WindowManager::closeSpeedLimitsWindow);
#endif
}

void WindowManager::setupGraphWindow()
{
    m_graphWindow = new GraphWindow();

    connect(m_graphWindow, &GraphWindow::aboutToClose, this, [&] { closeGraphWindow(); });
    connect(m_graphWindow, &GraphWindow::mouseRightClick, this,
            [&](QMouseEvent *event) { m_trayIcon->showTrayMenu(GuiUtil::globalPos(event)); });

    connect(IoC<StatManager>(), &StatManager::trafficAdded, m_graphWindow,
            &GraphWindow::addTraffic);
}

void WindowManager::setupStatisticsWindow()
{
    m_statWindow = new StatisticsWindow();

    connect(m_statWindow, &StatisticsWindow::aboutToClose, this,
            &WindowManager::closeStatisticsWindow);
}

void WindowManager::setupConfManager()
{
    auto confManager = IoCDependency<ConfManager>();

    connect(confManager, &ConfManager::iniUserChanged, this, &WindowManager::setupByIniUser);
}

void WindowManager::setupByIniUser(const IniUser &ini)
{
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
    if (graphWindow() && graphWindow()->isVisible())
        return;

    if (ini.graphWindowHideOnClose()) {
        if (!graphWindow()) {
            setupGraphWindow();
        }
    } else {
        closeGraphWindow();
    }
}

void WindowManager::setupTrayIcon()
{
    Q_ASSERT(!m_trayIcon);

    m_trayIcon = new TrayIcon(this);

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this,
            &WindowManager::onTrayMessageClicked, Qt::QueuedConnection);
}

void WindowManager::showTrayIcon()
{
    m_trayIcon->show();
}

void WindowManager::closeTrayIcon()
{
    m_trayIcon->hide();
}

void WindowManager::showTrayMessage(const QString &message, WindowManager::TrayMessageType type)
{
    if (!m_trayIcon)
        return;

    m_lastTrayMessageType = type;
    m_trayIcon->showMessage(QGuiApplication::applicationDisplayName(), message);
}

void WindowManager::showSplashScreen()
{
    auto splash = new SplashScreen(); // auto-deleted on close
    splash->showFading();
}

void WindowManager::showHomeWindow()
{
    if (!m_homeWindow) {
        setupHomeWindow();
    }

    showWindow(m_homeWindow);
}

void WindowManager::closeHomeWindow()
{
    if (closeWindow(m_homeWindow)) {
        m_homeWindow = nullptr;
    }
}

void WindowManager::quitHomeWindow(QEvent *event)
{
    if (trayIcon()->isVisible()) {
        closeHomeWindow();
        return;
    }

    if (m_isAppQuitting)
        return;

    event->ignore();

    trayIcon()->quitProgram();
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

bool WindowManager::showProgramsWindow()
{
    if (!checkWindowPassword(WindowPrograms))
        return false;

    if (!m_progWindow) {
        setupProgramsWindow();
    }

    showWindow(m_progWindow);

    return true;
}

void WindowManager::closeProgramsWindow()
{
    if (closeWindow(m_progWindow)) {
        m_progWindow = nullptr;
    }
}

bool WindowManager::showProgramEditForm(const QString &appPath)
{
    if (!showProgramsWindow())
        return false;

    if (!m_progWindow->editProgramByPath(appPath)) {
        showErrorBox(tr("Please close already opened Edit Program window and try again."));
        return false;
    }

    return true;
}

void WindowManager::openProgramEditForm(
        const QString &appPath, qint64 appId, FormWindow *parentForm)
{
    ProgramsWindow::openProgramByPath(appPath, appId, parentForm);
}

void WindowManager::showProgramAlertWindow(bool activate)
{
    if (!checkWindowPassword(WindowProgramAlert))
        return;

    if (!m_progAlertWindow) {
        setupProgramAlertWindow();
    }

    if (m_progAlertWindow->isNew()) {
        closeProgramAlertWindow();
        showProgramsWindow();
    } else {
        showWindow(m_progAlertWindow, activate || m_progAlertWindow->isAutoActive());
    }
}

void WindowManager::closeProgramAlertWindow()
{
    if (closeWindow(m_progAlertWindow)) {
        m_progAlertWindow = nullptr;
    }
}

void WindowManager::showOptionsWindow()
{
    if (!checkWindowPassword(WindowOptions))
        return;

    if (!m_optWindow) {
        setupOptionsWindow();
    }

    showWindow(m_optWindow);
}

void WindowManager::closeOptionsWindow()
{
    if (closeWindow(m_optWindow)) {
        m_optWindow = nullptr;
    }
}

void WindowManager::reloadOptionsWindow(const QString &reason)
{
    if (!m_optWindow)
        return;

    // Unsaved changes are lost
    closeOptionsWindow();

    // Show after new conf initialization
    QMetaObject::invokeMethod(this, &WindowManager::showOptionsWindow, Qt::QueuedConnection);

    showTrayMessage(reason);
}

void WindowManager::showRulesWindow()
{
    if (!checkWindowPassword(WindowRules))
        return;

    if (!m_rulesWindow) {
        setupRulesWindow();
    }

    showWindow(m_rulesWindow);
}

void WindowManager::closeRulesWindow()
{
    if (closeWindow(m_rulesWindow)) {
        m_rulesWindow = nullptr;
    }
}

void WindowManager::showStatisticsWindow()
{
    if (!checkWindowPassword(WindowStatistics))
        return;

    if (!m_statWindow) {
        setupStatisticsWindow();
    }

    showWindow(m_statWindow);
}

void WindowManager::closeStatisticsWindow()
{
    if (closeWindow(m_statWindow)) {
        m_statWindow = nullptr;
    }
}

void WindowManager::showOptionsWindowTab(int index)
{
    showOptionsWindow();

    if (m_optWindow) {
        m_optWindow->selectTab(index);
    }
}

void WindowManager::showAppGroupsWindow()
{
    showOptionsWindowTab(3);
}

void WindowManager::showServicesWindow()
{
    if (!checkWindowPassword(WindowServices))
        return;

    if (!m_servicesWindow) {
        setupServicesWindow();
    }

    showWindow(m_servicesWindow);
}

void WindowManager::closeServicesWindow()
{
    if (closeWindow(m_servicesWindow)) {
        m_servicesWindow = nullptr;
    }
}

void WindowManager::showZonesWindow()
{
    if (!checkWindowPassword(WindowZones))
        return;

    if (!m_zonesWindow) {
        setupZonesWindow();
    }

    showWindow(m_zonesWindow);
}

void WindowManager::closeZonesWindow()
{
    if (closeWindow(m_zonesWindow)) {
        m_zonesWindow = nullptr;
    }
}

void WindowManager::showGroupsWindow()
{
    if (!checkWindowPassword(WindowGroups))
        return;

    if (!m_groupsWindow) {
        setupGroupsWindow();
    }

    showWindow(m_groupsWindow);
}

void WindowManager::closeGroupsWindow()
{
    if (closeWindow(m_groupsWindow)) {
        m_groupsWindow = nullptr;
    }
}

void WindowManager::showSpeedLimitsWindow()
{
    if (!checkWindowPassword(WindowSpeedLimits))
        return;

    if (!m_speedLimitsWindow) {
        setupSpeedLimitsWindow();
    }

#if 0 // TODO
    showWindow(m_speedLimitsWindow);
#endif
}

void WindowManager::closeSpeedLimitsWindow()
{
#if 0 // TODO
    if (closeWindow(m_speedLimitsWindow)) {
        m_speedLimitsWindow = nullptr;
    }
#endif
}

void WindowManager::showGraphWindow()
{
    if (!m_graphWindow) {
        setupGraphWindow();
    }

    showWindow(m_graphWindow, /*activate=*/false);
}

void WindowManager::closeGraphWindow()
{
    if (closeWindow(m_graphWindow)) {
        m_graphWindow = nullptr;
    }
}

void WindowManager::showWindowByCode(WindowCode code)
{
    if (Q_UNLIKELY(code < WindowHome || code > WindowGraph))
        return;

    static const std::function<void()> showFuncs[] = {
        [&] { showHomeWindow(); },
        [&] { showProgramsWindow(); },
        [&] { showProgramAlertWindow(); },
        [&] { showServicesWindow(); },
        [&] { showOptionsWindow(); },
        [&] { showRulesWindow(); },
        [&] { showStatisticsWindow(); },
        [&] { showZonesWindow(); },
        [&] { showGroupsWindow(); },
        [&] { showSpeedLimitsWindow(); },
        [&] { showGraphWindow(); },
    };

    const int index = BitUtil::bitScanForward(code);

    showFuncs[index]();
}

void WindowManager::closeWindowByCode(WindowCode code)
{
    if (Q_UNLIKELY(code < WindowHome || code > WindowGraph))
        return;

    static const std::function<void()> closeFuncs[] = {
        [&] { closeHomeWindow(); },
        [&] { closeProgramsWindow(); },
        [&] { closeProgramAlertWindow(); },
        [&] { closeServicesWindow(); },
        [&] { closeOptionsWindow(); },
        [&] { closeRulesWindow(); },
        [&] { closeStatisticsWindow(); },
        [&] { closeZonesWindow(); },
        [&] { closeGroupsWindow(); },
        [&] { closeSpeedLimitsWindow(); },
        [&] { closeGraphWindow(); },
    };

    const int index = BitUtil::bitScanForward(code);

    closeFuncs[index]();
}

void WindowManager::switchWindowByCode(WindowCode code)
{
    if (isAnyWindowOpen(code)) {
        closeWindowByCode(code);
    } else {
        showWindowByCode(code);
    }
}

void WindowManager::closeAllWindows()
{
    closeGraphWindow();
    closeHomeWindow();
    closeProgramsWindow();
    closeOptionsWindow();
    closeRulesWindow();
    closeServicesWindow();
    closeZonesWindow();
    closeStatisticsWindow();
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

    const auto settings = IoC<FortSettings>();

    if (!settings->isPasswordRequired() || settings->passwordTemporaryChecked())
        return true;

    return checkPasswordDialog(code, settings);
}

void WindowManager::resetCheckedPassword()
{
    m_unlockedWindows = 0;

    IoC<FortSettings>()->resetCheckedPassword();
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
    auto box = DialogUtil::createMessageBox({ .icon = QMessageBox::Question,
                                                    .buttons = QMessageBox::Yes | QMessageBox::No,
                                                    .text = text,
                                                    .title = title },
            parent);

    connect(
            box, &QMessageBox::finished, this,
            [=](int result) {
                const bool confirmed = (result == QMessageBox::Yes);
                onFinished(confirmed);
            },
            Qt::QueuedConnection);

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

    if (!(ok && IoC<ConfManager>()->checkPassword(password)))
        return false;

    windowUnlocked(code);

    settings->setPasswordChecked(true, FortSettings::UnlockType(unlockType));

    return true;
}

void WindowManager::onTrayMessageClicked()
{
    switch (m_lastTrayMessageType) {
    case TrayMessageNewVersion: {
        showHomeWindowAbout();
    } break;
    case TrayMessageZones: {
        showZonesWindow();
    } break;
    case TrayMessageAlert: {
        showProgramAlertWindow();
    } break;
    default:
        showOptionsWindow();
    }
}

void WindowManager::showWindow(FormWindow *w, bool activate)
{
    w->showWindow(activate);

    windowOpened(w->windowCode());
}

bool WindowManager::closeWindow(FormWindow *w)
{
    if (!w) {
        return false;
    }

    if (w->isVisible()) {
        w->saveWindowState(m_isAppQuitting);
        w->hide();

        windowClosed(w->windowCode());

        if (!isAnyWindowUnlocked()) {
            IoC<FortSettings>()->resetCheckedPassword(FortSettings::UnlockWindow);
        }
    }

    if (m_isAppQuitting || w->deleteOnClose()) {
        emit w->aboutToDelete();
        w->deleteLater();
        return true;
    }

    return false;
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
