#include "homewindow.h"

#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <fort_version.h>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
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

HomeWindow::HomeWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new HomeController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
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

void HomeWindow::saveWindowState()
{
    iniUser()->setServiceWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setServiceWindowMaximized(m_stateWatcher->maximized());

    emit ctrl()->afterSaveWindowState(iniUser());

    confManager()->saveIniUser();
}

void HomeWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(600, 400), iniUser()->homeWindowGeometry(),
            iniUser()->homeWindowMaximized());

    emit ctrl()->afterRestoreWindowState(iniUser());
}

void HomeWindow::showMenu()
{
    m_btMenu->showMenu();
}

void HomeWindow::selectAboutTab()
{
    m_mainPage->setCurrentTab(HomeMainPage::TabAbout);
}

void HomeWindow::retranslateUi()
{
    this->unsetLocale();

    m_btPasswordLock->setText(tr("Lock"));
    m_btPasswordUnlock->setText(tr("Unlock"));
    m_btMenu->setText(tr("Menu"));

    m_btLogs->setText(tr("Logs"));
    m_btProfile->setText(tr("Profile"));
    m_btStat->setText(tr("Statistics"));
    m_btReleases->setText(tr("Releases"));

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
    this->setMinimumSize(500, 400);
}

QWidget *HomeWindow::setupHeader()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x26, 0x26, 0x26));

    auto frame = new QWidget();
    frame->setAutoFillBackground(true);
    frame->setPalette(palette);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(10);

    // Logo image
    auto iconLogo = ControlUtil::createLabel();
    iconLogo->setScaledContents(true);
    iconLogo->setMaximumSize(48, 48);
    iconLogo->setPixmap(IconCache::file(":/icons/fort-96.png"));

    // Logo text
    auto textLogo = setupLogoText();

    // Password Unlock button
    setupPasswordButtons();

    // Menu button
    m_btMenu = ControlUtil::createButton(":/icons/large_tiles.png");
    m_btMenu->setMenu(windowManager()->trayIcon()->menu());

    layout->addWidget(iconLogo);
    layout->addLayout(textLogo);
    layout->addStretch();
    layout->addWidget(m_btPasswordLock);
    layout->addWidget(m_btPasswordUnlock);
    layout->addWidget(m_btMenu);

    frame->setLayout(layout);

    return frame;
}

QLayout *HomeWindow::setupLogoText()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::white);

    QFont font("Franklin Gothic", 22, QFont::Bold);

    // Label
    auto label = ControlUtil::createLabel(QGuiApplication::applicationName());
    label->setPalette(palette);
    label->setFont(font);

    // Sub-label
    font.setPointSize(10);
    font.setWeight(QFont::DemiBold);

    auto subLabel = ControlUtil::createLabel("- keeping you secure -");
    subLabel->setPalette(palette);
    subLabel->setFont(font);

    layout->addWidget(label, 2, Qt::AlignHCenter | Qt::AlignBottom);
    layout->addWidget(subLabel, 1, Qt::AlignHCenter | Qt::AlignTop);

    return layout;
}

void HomeWindow::setupPasswordButtons()
{
    m_btPasswordLock = ControlUtil::createToolButton(
            ":/icons/lock.png", [&] { ctrl()->setPasswordLocked(true); });

    m_btPasswordUnlock = ControlUtil::createToolButton(":/icons/lock_open.png",
            [&] { ctrl()->setPasswordLocked(!windowManager()->checkPassword()); });

    const auto refreshPasswordButtons = [&] {
        m_btPasswordLock->setVisible(settings()->hasPassword() && !ctrl()->passwordLocked());
        m_btPasswordUnlock->setVisible(ctrl()->passwordLocked());
    };

    refreshPasswordButtons();

    connect(ctrl(), &HomeController::passwordLockedChanged, this, refreshPasswordButtons);
}

QLayout *HomeWindow::setupDialogButtons()
{
    auto layout = new QHBoxLayout();

    m_btLogs = ControlUtil::createLinkButton(":/icons/folder.png", settings()->logsPath());
    m_btProfile = ControlUtil::createLinkButton(":/icons/folder.png", settings()->profilePath());
    m_btStat = ControlUtil::createLinkButton(":/icons/folder.png", settings()->statPath());
    m_btReleases = ControlUtil::createLinkButton(":/icons/github.png", APP_UPDATES_URL);

    connect(m_btLogs, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btProfile, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btStat, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);
    connect(m_btReleases, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);

    layout->addWidget(m_btLogs);
    layout->addWidget(m_btProfile);
    layout->addWidget(m_btStat);
    layout->addWidget(m_btReleases);

    layout->addStretch();

    return layout;
}
