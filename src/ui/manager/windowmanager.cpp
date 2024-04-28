#include "windowmanager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/mainwindow.h>
#include <form/dialog/dialogutil.h>
#include <form/dialog/passworddialog.h>
#include <form/dialog/splashscreen.h>
#include <form/graph/graphwindow.h>
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
#include <util/guiutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "nativeeventfilter.h"

namespace {

const QLoggingCategory LC("manager.window");

void setupAppStyle()
{
    QStyle *style = QStyleFactory::create("Fusion");
    QApplication::setStyle(style);
}

inline bool isWindowVisible(WidgetWindow *w)
{
    return w && w->isVisible();
}

}

WindowManager::WindowManager(QObject *parent) : QObject(parent) { }

void WindowManager::setUp()
{
    setupAppStyle();
    setupAppPalette();

    setupMainWindow();
    setupConfManager();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &WindowManager::setupAppPalette);
#endif

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

QPushButton *WindowManager::createMenuButton() const
{
    auto c = ControlUtil::createButton(":/icons/large_tiles.png");
    c->setMenu(trayIcon()->menu());
    return c;
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
    const QPalette palette = QApplication::style()->standardPalette();

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
    m_homeWindow->restoreWindowState();

    connect(m_homeWindow, &HomeWindow::aboutToClose, this, &WindowManager::quitHomeWindow);
}

void WindowManager::setupProgramsWindow()
{
    m_progWindow = new ProgramsWindow();
    m_progWindow->restoreWindowState();

    connect(m_progWindow, &ProgramsWindow::aboutToClose, this, &WindowManager::closeProgramsWindow);
}

void WindowManager::setupProgramAlertWindow()
{
    m_progAlertWindow = new ProgramAlertWindow();
    m_progAlertWindow->restoreWindowState();

    connect(m_progAlertWindow, &ProgramAlertWindow::aboutToClose, this,
            &WindowManager::closeProgramAlertWindow);
}

void WindowManager::setupOptionsWindow()
{
    m_optWindow = new OptionsWindow();
    m_optWindow->restoreWindowState();

    connect(m_optWindow, &OptionsWindow::aboutToClose, this, &WindowManager::closeOptionsWindow);
}

void WindowManager::setupRulesWindow()
{
    m_rulesWindow = new RulesWindow();
    m_rulesWindow->restoreWindowState();

    connect(m_rulesWindow, &RulesWindow::aboutToClose, this, &WindowManager::closeRulesWindow);
}

void WindowManager::setupServicesWindow()
{
    m_servicesWindow = new ServicesWindow();
    m_servicesWindow->restoreWindowState();

    connect(m_servicesWindow, &ServicesWindow::aboutToClose, this,
            &WindowManager::closeServicesWindow);
}

void WindowManager::setupZonesWindow()
{
    m_zonesWindow = new ZonesWindow();
    m_zonesWindow->restoreWindowState();

    connect(m_zonesWindow, &ZonesWindow::aboutToClose, this, &WindowManager::closeZonesWindow);
}

void WindowManager::setupGraphWindow()
{
    m_graphWindow = new GraphWindow();
    m_graphWindow->restoreWindowState();

    connect(m_graphWindow, &GraphWindow::aboutToClose, this, [&] { closeGraphWindow(); });
    connect(m_graphWindow, &GraphWindow::mouseRightClick, this,
            [&](QMouseEvent *event) { m_trayIcon->showTrayMenu(GuiUtil::globalPos(event)); });

    connect(IoC<StatManager>(), &StatManager::trafficAdded, m_graphWindow,
            &GraphWindow::addTraffic);
}

void WindowManager::setupStatisticsWindow()
{
    m_statWindow = new StatisticsWindow();
    m_statWindow->restoreWindowState();

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
    updateTrayIconVisibility(ini);
    updateGraphWindowVisibility(ini);
}

void WindowManager::updateTrayIconVisibility(const IniUser &ini)
{
    if (ini.trayShowIcon()) {
        showTrayIcon();
    } else {
        closeTrayIcon();

        if (!homeWindow()) {
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

void WindowManager::showProgramAlertWindow()
{
    if (!checkWindowPassword(WindowProgramAlert))
        return;

    if (!m_progAlertWindow) {
        setupProgramAlertWindow();
    }

    if (m_progAlertWindow->isEmpty()) {
        closeProgramAlertWindow();
        showProgramsWindow();
    } else {
        showWindow(m_progAlertWindow);
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
        m_optWindow->cancelChanges();
        m_optWindow = nullptr;
    }
}

void WindowManager::reloadOptionsWindow(const QString &reason)
{
    if (!m_optWindow)
        return;

    // Unsaved changes are lost
    closeOptionsWindow();
    showOptionsWindow();

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
    showOptionsWindowTab(2);
}

void WindowManager::showStatOptionsWindow()
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

void WindowManager::switchGraphWindow()
{
    if (isWindowVisible(m_graphWindow)) {
        closeGraphWindow();
    } else {
        showGraphWindow();
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
    return (WindowPasswordProtected & code) == 0 || checkPassword();
}

bool WindowManager::checkPassword(bool temporary)
{
    if (isAnyWindowOpen(WindowPasswordDialog)) {
        activateModalWidget();
        return false;
    }

    if (isAnyWindowOpen(WindowPasswordProtected))
        return true;

    const auto settings = IoC<FortSettings>();

    if (!settings->isPasswordRequired())
        return true;

    windowOpened(WindowPasswordDialog);

    QString password;
    int unlockType = FortSettings::UnlockDisabled;
    bool passwordChecked = false;

    if (showPasswordDialog(password, &unlockType) // input the password
            && IoC<ConfManager>()->checkPassword(password)) {

        if (!temporary || unlockType != FortSettings::UnlockWindow) {
            settings->setPasswordChecked(
                    /*checked=*/true, FortSettings::UnlockType(unlockType));
        }

        passwordChecked = true;
    }

    windowClosed(WindowPasswordDialog);

    return passwordChecked;
}

void WindowManager::showErrorBox(const QString &text, const QString &title, QWidget *parent)
{
    showErrorDialog(text, title, parent);
}

void WindowManager::showInfoBox(const QString &text, const QString &title, QWidget *parent)
{
    showInfoDialog(text, title, parent);
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

void WindowManager::showErrorDialog(const QString &text, const QString &title, QWidget *parent)
{
    auto box = DialogUtil::createMessageBox(
            {
                    .icon = QMessageBox::Warning,
                    .buttons = QMessageBox::Ok,
                    .text = text,
                    .title = title,
            },
            parent);

    DialogUtil::showDialog(box);
}

void WindowManager::showInfoDialog(const QString &text, const QString &title, QWidget *parent)
{
    auto box = DialogUtil::createMessageBox(
            {
                    .icon = QMessageBox::Information,
                    .buttons = QMessageBox::Ok,
                    .text = text,
                    .title = title,
            },
            parent);

    DialogUtil::showDialog(box);
}

bool WindowManager::showPasswordDialog(QString &password, int *unlockType)
{
    auto box = new PasswordDialog();
    QScopedPointer<PasswordDialog> deferBox(box);

    DialogUtil::setupModalDialog(box);

    DialogUtil::showDialog(box);

    if (box->exec() != QDialog::Accepted)
        return false;

    password = box->password();
    if (password.isEmpty())
        return false;

    if (unlockType) {
        *unlockType = box->unlockType();
    }

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

void WindowManager::showWindow(WidgetWindow *w, bool activate)
{
    w->showWindow(activate);

    windowOpened(w->windowCode());
}

bool WindowManager::closeWindow(WidgetWindow *w)
{
    if (!w) {
        return false;
    }

    if (w->isVisible()) {
        w->saveWindowState(m_isAppQuitting);
        w->hide();

        windowClosed(w->windowCode());

        if (!isAnyWindowOpen(WindowPasswordProtected)) {
            IoC<FortSettings>()->resetCheckedPassword(FortSettings::UnlockWindow);
        }
    }

    if (m_isAppQuitting || w->deleteOnClose()) {
        w->deleteLater();
        return true;
    }

    return false;
}

void WindowManager::windowOpened(quint32 code)
{
    m_openedWindows |= code;

    emit windowVisibilityChanged(code, /*isVisible=*/true);
}

void WindowManager::windowClosed(quint32 code)
{
    m_openedWindows &= ~code;

    emit windowVisibilityChanged(code, /*isVisible=*/false);
}

bool WindowManager::isAnyWindowOpen(quint32 codes) const
{
    return (m_openedWindows & codes) != 0;
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
