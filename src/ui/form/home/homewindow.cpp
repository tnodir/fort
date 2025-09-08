#include "homewindow.h"

#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <fort_version.h>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/dialog/splashscreen.h>
#include <form/tray/trayicon.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "homecontroller.h"
#include "pages/homemainpage.h"

namespace {

QToolButton *createFlatToolButton(const QString &iconPath, const QString &linkPath)
{
    auto c = ControlUtil::createFlatToolButton(iconPath);
    c->setWindowFilePath(linkPath);
    c->setToolTip(linkPath);
    return c;
}

}

HomeWindow::HomeWindow(QWidget *parent) : FormWindow(parent), m_ctrl(new HomeController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::homeWindowGroup());

    connect(this, &HomeWindow::activationChanged, this, &HomeWindow::onActivationChanged,
            Qt::QueuedConnection); // queued to properly show the menu after window opening
}

FortSettings *HomeWindow::settings() const
{
    return ctrl()->settings();
}

ConfManager *HomeWindow::confManager() const
{
    return ctrl()->confManager();
}

IniUser *HomeWindow::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *HomeWindow::windowManager() const
{
    return ctrl()->windowManager();
}

void HomeWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setHomeWindowGeometry(stateWatcher()->geometry());
    iniUser()->setHomeWindowMaximized(stateWatcher()->maximized());

    emit ctrl() -> afterSaveWindowState(iniUser());

    confManager()->saveIniUser();
}

void HomeWindow::restoreWindowState()
{
    stateWatcher()->restore(this, QSize(600, 400), iniUser()->homeWindowGeometry(),
            iniUser()->homeWindowMaximized());

    emit ctrl() -> afterRestoreWindowState(iniUser());
}

void HomeWindow::selectAboutTab()
{
    m_mainPage->setCurrentTab(HomeMainPage::TabAbout);
}

void HomeWindow::onAboutToClose(QEvent *event)
{
    if (!iniUser()->homeWindowAutoShowWindow())
        return;

    auto windowManager = this->windowManager();
    auto trayIcon = windowManager->trayIcon();

    if (trayIcon->isVisible()) {
        FormWindow::onAboutToClose(event);
        return;
    }

    if (windowManager->isAppQuitting())
        return;

    event->ignore();

    trayIcon->quitProgram();
}

void HomeWindow::onActivationChanged(bool isActive)
{
    if (isActive && iniUser()->homeWindowAutoShowMenu()) {
        if (isActiveWindow()) {
            m_btMenu->showMenu();
        }
    }
}

void HomeWindow::retranslateUi()
{
    this->unsetLocale();

    m_btPasswordLock->setText(tr("Lock"));
    m_btPasswordUnlock->setText(tr("Unlock"));

    m_btProfile->setText(tr("Profile"));
    m_btLogs->setText(tr("Logs"));
    m_btServiceLogs->setText(tr("Service Logs"));
    m_btReleases->setText(tr("Releases"));
    m_btHelp->setText(tr("Help"));

    this->setWindowTitle(tr("My Fort"));
}

void HomeWindow::setupController()
{
    connect(ctrl(), &HomeController::retranslateUi, this, &HomeWindow::retranslateUi);

    emit ctrl() -> retranslateUi();
}

void HomeWindow::setupUi()
{
    auto layout = ControlUtil::createVLayout();
    layout->setSpacing(0);

    // Header
    auto header = setupHeader();
    layout->addWidget(header);

    // Main page
    m_mainPage = new HomeMainPage(ctrl());
    layout->addWidget(m_mainPage, 1);

    // Dialog buttons
    auto buttonsLayout = setupDialogButtons();
    layout->addLayout(buttonsLayout);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 300);
}

QWidget *HomeWindow::setupHeader()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x26, 0x26, 0x26));

    auto frame = new QWidget();
    frame->setAutoFillBackground(true);
    frame->setPalette(palette);

    // Logo Layout
    auto logoLayout = SplashScreen::createLogoLayout();

    // Buttons
    setupHeaderButtons();

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(10, 0, 6, 0);

    layout->addLayout(logoLayout);
    layout->addStretch();
    layout->addWidget(m_btPasswordLock);
    layout->addWidget(m_btPasswordUnlock);
    layout->addWidget(m_btOptions);
    layout->addWidget(m_btMenu);

    frame->setLayout(layout);

    return frame;
}

void HomeWindow::setupHeaderButtons()
{
    // Password Unlock button
    setupPasswordButtons();

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();
}

void HomeWindow::setupPasswordButtons()
{
    m_btPasswordLock = ControlUtil::createToolButton(
            ":/icons/lock.png", [&] { windowManager()->resetCheckedPassword(); });

    m_btPasswordUnlock = ControlUtil::createToolButton(":/icons/lock_open.png");

    connect(
            m_btPasswordUnlock, &QToolButton::clicked, this,
            [&] { windowManager()->checkPassword(WindowHome); }, Qt::QueuedConnection);

    const auto refreshPasswordButtons = [&] {
        m_btPasswordLock->setVisible(settings()->hasPassword() && !ctrl()->passwordLocked());
        m_btPasswordUnlock->setVisible(ctrl()->passwordLocked());
    };

    refreshPasswordButtons();

    connect(ctrl(), &HomeController::passwordLockedChanged, this, refreshPasswordButtons);
}

QLayout *HomeWindow::setupDialogButtons()
{
    m_btProfile = createFlatToolButton(":/icons/folder.png", settings()->profilePath());
    m_btLogs = createFlatToolButton(":/icons/folder.png", settings()->logsPath());
    m_btServiceLogs = createFlatToolButton(":/icons/folder.png", settings()->profileLogsPath());
    m_btReleases = createFlatToolButton(":/icons/github.png", APP_UPDATES_URL);
    m_btHelp = createFlatToolButton(":/icons/help.png", "https://github.com/tnodir/fort/wiki");

    m_btServiceLogs->setVisible(
            settings()->hasService() && settings()->profileLogsPath() != settings()->logsPath());

    connect(m_btProfile, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btLogs, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btServiceLogs, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btReleases, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btHelp, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btProfile, m_btLogs, m_btServiceLogs,
            m_btReleases, m_btHelp, /*stretch*/ nullptr });
    layout->setContentsMargins(6, 4, 6, 4);

    return layout;
}
