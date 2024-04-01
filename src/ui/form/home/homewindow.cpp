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

HomeWindow::HomeWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new HomeController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();

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
    iniUser()->setHomeWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setHomeWindowMaximized(m_stateWatcher->maximized());

    emit ctrl()->afterSaveWindowState(iniUser());

    confManager()->saveIniUser();
}

void HomeWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(600, 400), iniUser()->homeWindowGeometry(),
            iniUser()->homeWindowMaximized());

    emit ctrl()->afterRestoreWindowState(iniUser());
}

void HomeWindow::selectAboutTab()
{
    m_mainPage->setCurrentTab(HomeMainPage::TabAbout);
}

void HomeWindow::onActivationChanged(bool isActive)
{
    if (isActive && iniUser()->homeAutoShowMenu()) {
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
    m_btStat->setText(tr("Statistics"));
    m_btReleases->setText(tr("Releases"));
    m_btHelp->setText(tr("Help"));

    this->setWindowTitle(tr("My Fort"));
}

void HomeWindow::setupController()
{
    connect(ctrl(), &HomeController::retranslateUi, this, &HomeWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void HomeWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void HomeWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
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

    // Icon
    this->setWindowIcon(IconCache::icon(":/icons/fort.png"));

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

    // Logo icon
    auto logoIcon = SplashScreen::createLogoIcon();

    // Logo text
    auto logoText = SplashScreen::createLogoTextLayout();

    // Password Unlock button
    setupPasswordButtons();

    // Menu button
    m_btMenu = windowManager()->createMenuButton();

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(16, 6, 6, 6);
    layout->setSpacing(10);

    layout->addWidget(logoIcon);
    layout->addLayout(logoText);
    layout->addStretch();
    layout->addWidget(m_btPasswordLock);
    layout->addWidget(m_btPasswordUnlock);
    layout->addWidget(m_btMenu);

    frame->setLayout(layout);

    return frame;
}

void HomeWindow::setupPasswordButtons()
{
    m_btPasswordLock = ControlUtil::createToolButton(
            ":/icons/lock.png", [&] { settings()->resetCheckedPassword(); });

    m_btPasswordUnlock = ControlUtil::createToolButton(
            ":/icons/lock_open.png", [&] { windowManager()->checkPassword(); });

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
    m_btStat = createFlatToolButton(":/icons/folder.png", settings()->statPath());
    m_btReleases = createFlatToolButton(":/icons/github.png", APP_UPDATES_URL);
    m_btHelp = createFlatToolButton(
            ":/icons/help.png", "https://github.com/tnodir/fort/wiki/User-Guide");

    connect(m_btProfile, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btLogs, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btStat, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btReleases, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btHelp, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btProfile, m_btLogs, m_btStat, m_btReleases, m_btHelp, /*stretch*/ nullptr });
    layout->setContentsMargins(6, 4, 6, 4);

    return layout;
}
