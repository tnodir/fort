#include "windowmanager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

#include <conf/confmanager.h>
#include <form/controls/mainwindow.h>
#include <form/dialog/passworddialog.h>
#include <form/graph/graphwindow.h>
#include <form/home/homewindow.h>
#include <form/opt/optionswindow.h>
#include <form/policy/policieswindow.h>
#include <form/prog/programswindow.h>
#include <form/stat/statisticswindow.h>
#include <form/svc/serviceswindow.h>
#include <form/tray/trayicon.h>
#include <form/zone/zoneswindow.h>
#include <fortcompat.h>
#include <fortsettings.h>
#include <stat/statmanager.h>
#include <user/usersettings.h>
#include <util/ioc/ioccontainer.h>

#include "nativeeventfilter.h"

namespace {

const QLoggingCategory LC("manager.window");

void setupAppStyle()
{
    QStyle *style = QStyleFactory::create("Fusion");
    QApplication::setStyle(style);
}

struct MessageBoxArg
{
    QMessageBox::Icon icon;
    QMessageBox::StandardButtons buttons;
    const QString text;
    const QString title;
};

QMessageBox *createMessageBox(const MessageBoxArg &ba, QWidget *parent)
{
    auto box = new QMessageBox(ba.icon, ba.title, ba.text, ba.buttons, parent);
    box->setAttribute(Qt::WA_DeleteOnClose);

    box->setWindowModality(parent ? Qt::WindowModal : Qt::ApplicationModal);
    box->setModal(true);

    return box;
}

}

WindowManager::WindowManager(QObject *parent) : QObject(parent) { }

void WindowManager::setUp()
{
    IoC()->setUpDependency<UserSettings>();
    IoC()->setUpDependency<NativeEventFilter>();

    setupAppStyle();
    setupAppPalette();

    setupMainWindow();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &WindowManager::setupAppPalette);
#endif

    connect(qApp, &QCoreApplication::aboutToQuit, this, &WindowManager::closeAll);
}

void WindowManager::tearDown()
{
    closeAll();
}

void WindowManager::showWidget(QWidget *w)
{
    if (w->isMinimized()) {
        w->setWindowState(w->windowState() ^ Qt::WindowMinimized);
    }
    w->show();
    w->raise();
    w->activateWindow();
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
    QApplication::setPalette(QApplication::style()->standardPalette());
}

void WindowManager::setupMainWindow()
{
    m_mainWindow = new MainWindow();

    // Font
    m_mainWindow->setFont(defaultFont());

    // Register Native events
    auto nativeEventFilter = IoC<NativeEventFilter>();

    nativeEventFilter->registerSessionNotification(mainWindow()->winId());

    connect(nativeEventFilter, &NativeEventFilter::sessionLocked, this, [&] {
        IoC<FortSettings>()->resetCheckedPassword(PasswordDialog::UnlockTillSessionLock);
    });
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

    connect(m_homeWindow, &HomeWindow::aboutToClose, this, &WindowManager::closeHomeWindow);
}

void WindowManager::setupProgramsWindow()
{
    m_progWindow = new ProgramsWindow();
    m_progWindow->restoreWindowState();

    connect(m_progWindow, &ProgramsWindow::aboutToClose, this, &WindowManager::closeProgramsWindow);
    connect(m_progWindow, &ProgramsWindow::activationChanged, m_trayIcon,
            [&] { m_trayIcon->updateTrayIcon(/*alerted=*/false); });
}

void WindowManager::setupOptionsWindow()
{
    m_optWindow = new OptionsWindow();
    m_optWindow->restoreWindowState();

    connect(m_optWindow, &OptionsWindow::aboutToClose, this, &WindowManager::closeOptionsWindow);
}

void WindowManager::setupPoliciesWindow()
{
    m_policiesWindow = new PoliciesWindow();
    m_policiesWindow->restoreWindowState();

    connect(m_policiesWindow, &PoliciesWindow::aboutToClose, this,
            &WindowManager::closePoliciesWindow);
}

void WindowManager::setupServicesWindow()
{
    m_serviceWindow = new ServicesWindow();
    m_serviceWindow->restoreWindowState();

    connect(m_serviceWindow, &ServicesWindow::aboutToClose, this,
            &WindowManager::closeServicesWindow);
}

void WindowManager::setupZonesWindow()
{
    m_zoneWindow = new ZonesWindow();
    m_zoneWindow->restoreWindowState();

    connect(m_zoneWindow, &ZonesWindow::aboutToClose, this, &WindowManager::closeZonesWindow);
}

void WindowManager::setupGraphWindow()
{
    m_graphWindow = new GraphWindow();
    m_graphWindow->restoreWindowState();

    connect(m_graphWindow, &GraphWindow::aboutToClose, this, [&] { closeGraphWindow(); });
    connect(m_graphWindow, &GraphWindow::mouseRightClick, this,
            [&](QMouseEvent *event) { m_trayIcon->showTrayMenu(mouseEventGlobalPos(event)); });

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

void WindowManager::closeAll()
{
    closeGraphWindow(true);
    closeProgramsWindow();
    closeOptionsWindow();
    closePoliciesWindow();
    closeServicesWindow();
    closeZonesWindow();
    closeStatisticsWindow();
    closeTrayIcon();
    closeMainWindow();
}

void WindowManager::setupTrayIcon()
{
    if (m_trayIcon)
        return;

    m_trayIcon = new TrayIcon(this);

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this,
            &WindowManager::onTrayMessageClicked);

    auto confManager = IoC<ConfManager>();
    connect(confManager, &ConfManager::appAlerted, m_trayIcon,
            [&] { m_trayIcon->updateTrayIcon(/*alerted=*/true); });
}

void WindowManager::showTrayIcon()
{
    setupTrayIcon();

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

    m_lastMessageType = type;
    m_trayIcon->showMessage(QGuiApplication::applicationDisplayName(), message);
}

void WindowManager::showHomeWindow()
{
    if (!widgetVisibleByCheckPassword(m_homeWindow))
        return;

    if (!m_homeWindow) {
        setupHomeWindow();
    }

    showWidget(m_homeWindow);
}

void WindowManager::closeHomeWindow()
{
    if (!m_homeWindow)
        return;

    m_homeWindow->saveWindowState();
    m_homeWindow->hide();

    m_homeWindow->deleteLater();
    m_homeWindow = nullptr;

    if (!trayIcon()->isVisible()) {
        trayIcon()->quitProgram();
    }
}

void WindowManager::showProgramsWindow()
{
    if (!widgetVisibleByCheckPassword(m_progWindow))
        return;

    if (!m_progWindow) {
        setupProgramsWindow();
    }

    showWidget(m_progWindow);
}

void WindowManager::closeProgramsWindow()
{
    if (!m_progWindow)
        return;

    m_progWindow->saveWindowState();
    m_progWindow->hide();

    m_progWindow->deleteLater();
    m_progWindow = nullptr;
}

bool WindowManager::showProgramEditForm(const QString &appPath)
{
    showProgramsWindow();
    if (!(m_progWindow && m_progWindow->isVisible()))
        return false; // May be not opened due to password checking

    if (!m_progWindow->editProgramByPath(appPath)) {
        showErrorBox(tr("Please close already opened Edit Program window and try again."));
        return false;
    }
    return true;
}

void WindowManager::showOptionsWindow()
{
    if (!widgetVisibleByCheckPassword(m_optWindow))
        return;

    if (!m_optWindow) {
        setupOptionsWindow();

        emit optWindowChanged(true);
    }

    showWidget(m_optWindow);
}

void WindowManager::closeOptionsWindow()
{
    if (!m_optWindow)
        return;

    m_optWindow->cancelChanges();
    m_optWindow->saveWindowState();
    m_optWindow->hide();

    m_optWindow->deleteLater();
    m_optWindow = nullptr;

    emit optWindowChanged(false);
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

void WindowManager::showPoliciesWindow()
{
    if (!widgetVisibleByCheckPassword(m_policiesWindow))
        return;

    if (!m_policiesWindow) {
        setupPoliciesWindow();
    }

    showWidget(m_policiesWindow);
}

void WindowManager::closePoliciesWindow()
{
    if (!m_policiesWindow)
        return;

    m_policiesWindow->saveWindowState();
    m_policiesWindow->hide();

    m_policiesWindow->deleteLater();
    m_policiesWindow = nullptr;
}

void WindowManager::showStatisticsWindow()
{
    if (!widgetVisibleByCheckPassword(m_statWindow))
        return;

    if (!m_statWindow) {
        setupStatisticsWindow();
    }

    showWidget(m_statWindow);
}

void WindowManager::closeStatisticsWindow()
{
    if (!m_statWindow)
        return;

    m_statWindow->saveWindowState();
    m_statWindow->hide();

    m_statWindow->deleteLater();
    m_statWindow = nullptr;
}

void WindowManager::showServicesWindow()
{
    if (!widgetVisibleByCheckPassword(m_serviceWindow))
        return;

    if (!m_serviceWindow) {
        setupServicesWindow();
    }

    showWidget(m_serviceWindow);
}

void WindowManager::closeServicesWindow()
{
    if (!m_serviceWindow)
        return;

    m_serviceWindow->saveWindowState();
    m_serviceWindow->hide();

    m_serviceWindow->deleteLater();
    m_serviceWindow = nullptr;
}

void WindowManager::showZonesWindow()
{
    if (!widgetVisibleByCheckPassword(m_zoneWindow))
        return;

    if (!m_zoneWindow) {
        setupZonesWindow();
    }

    showWidget(m_zoneWindow);
}

void WindowManager::closeZonesWindow()
{
    if (!m_zoneWindow)
        return;

    m_zoneWindow->saveWindowState();
    m_zoneWindow->hide();

    m_zoneWindow->deleteLater();
    m_zoneWindow = nullptr;
}

void WindowManager::showGraphWindow()
{
    if (!m_graphWindow) {
        setupGraphWindow();

        emit graphWindowChanged(true);
    }

    m_graphWindow->show();
}

void WindowManager::closeGraphWindow(bool wasVisible)
{
    if (!m_graphWindow)
        return;

    m_graphWindow->saveWindowState(wasVisible);
    m_graphWindow->hide();

    m_graphWindow->deleteLater();
    m_graphWindow = nullptr;

    emit graphWindowChanged(false);
}

void WindowManager::switchGraphWindow()
{
    if (!m_graphWindow) {
        showGraphWindow();
    } else {
        closeGraphWindow();
    }
}

void WindowManager::quit()
{
    closeAll();

    qCDebug(LC) << "Quit due user request";

    QCoreApplication::quit();
}

void WindowManager::restart()
{
    const QString appFilePath = QCoreApplication::applicationFilePath();
    const QStringList args = IoC<FortSettings>()->appArguments();

    connect(qApp, &QObject::destroyed, [=] { QProcess::startDetached(appFilePath, args); });

    qCDebug(LC) << "Quit due required restart";

    QCoreApplication::quit();
}

bool WindowManager::widgetVisibleByCheckPassword(QWidget *w)
{
    return (w && w->isVisible()) || checkPassword();
}

bool WindowManager::checkPassword()
{
    static bool g_passwordDialogOpened = false;

    const auto settings = IoC<FortSettings>();

    if (!settings->isPasswordRequired())
        return true;

    if (g_passwordDialogOpened) {
        activateModalWidget();
        return false;
    }

    g_passwordDialogOpened = true;

    QString password;
    PasswordDialog::UnlockType unlockType = PasswordDialog::UnlockDisabled;
    const bool ok = PasswordDialog::getPassword(password, unlockType, mainWindow());

    g_passwordDialogOpened = false;

    const bool checked = ok && !password.isEmpty() && IoC<ConfManager>()->checkPassword(password);

    settings->setPasswordChecked(checked, unlockType);

    return checked;
}

void WindowManager::showErrorBox(const QString &text, const QString &title, QWidget *parent)
{
    auto box = createMessageBox(
            {
                    .icon = QMessageBox::Warning,
                    .buttons = QMessageBox::Ok,
                    .text = text,
                    .title = title,
            },
            parent);
    box->show();
}

void WindowManager::showInfoBox(const QString &text, const QString &title, QWidget *parent)
{
    auto box = createMessageBox(
            {
                    .icon = QMessageBox::Information,
                    .buttons = QMessageBox::Ok,
                    .text = text,
                    .title = title,
            },
            parent);
    box->show();
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
    auto box = createMessageBox(
            {
                    .icon = QMessageBox::Question,
                    .buttons = QMessageBox::Yes | QMessageBox::Cancel,
                    .text = text,
                    .title = title,
            },
            parent);

    connect(box, &QMessageBox::finished, [=](int result) {
        const bool confirmed = (result == QMessageBox::Yes);
        onFinished(confirmed);
    });

    box->show();
}

void WindowManager::onTrayMessageClicked()
{
    switch (m_lastMessageType) {
    case MessageZones:
        showZonesWindow();
        break;
    default:
        showOptionsWindow();
    }
}

bool WindowManager::activateModalWidget()
{
    auto w = QApplication::activeModalWidget();
    if (w) {
        w->activateWindow();
        return true;
    }
    return false;
}
