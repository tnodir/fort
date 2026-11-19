#include "homewindow.h"

#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/headerbuttons.h>
#include <form/controls/logowidget.h>
#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "homecontroller.h"
#include "homelinks.h"
#include "pages/homemainpage.h"

using namespace Fort;

HomeWindow::HomeWindow(QWidget *parent) : FormWindow(parent), m_ctrl(new HomeController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser());

    connect(this, &HomeWindow::activationChanged, this, &HomeWindow::onActivationChanged,
            Qt::QueuedConnection); // queued to properly show the menu after window opening
}

QString HomeWindow::iniGroup() const
{
    return IniUser::homeWindowGroup();
}

void HomeWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    saveIniWindowGeometry();

    emit ctrl()->afterSaveWindowState(iniUser);

    confManager()->saveIniUser();
}

void HomeWindow::restoreWindowState()
{
    auto &iniUser = Fort::iniUser();

    restoreIniWindowGeometry(QSize(600, 500));

    emit ctrl()->afterRestoreWindowState(iniUser);
}

void HomeWindow::selectAboutTab()
{
    m_mainPage->setCurrentTab(HomeMainPage::TabAbout);
}

bool HomeWindow::checkAboutToClose()
{
    if (!iniUser().homeWindowAutoShowWindow())
        return true;

    auto windowManager = Fort::windowManager();
    auto trayIcon = windowManager->trayIcon();

    if (trayIcon->isVisible())
        return true;

    if (windowManager->isAppQuitting())
        return true;

    emit windowManager->requestQuit();

    return false;
}

void HomeWindow::onActivationChanged(bool isActive)
{
    if (isActive && iniUser().homeWindowAutoShowMenu()) {
        if (isActiveWindow()) {
            m_headerButtons->menuButton()->showMenu();
        }
    }
}

void HomeWindow::retranslateUi()
{
    this->unsetLocale();

    m_btPasswordLock->setText(tr("Lock"));
    m_btPasswordUnlock->setText(tr("Unlock"));

    for (int i = 0; i < HomeLinks::LinkCount; ++i) {
        m_linkButtons[i]->setText(HomeLinks::text(HomeLinks::Link(i)));
    }

    this->setWindowTitle(tr("My Fort"));
}

void HomeWindow::setupController()
{
    connect(ctrl(), &HomeController::retranslateUi, this, &HomeWindow::retranslateUi);

    emit ctrl()->retranslateUi();
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
    // Logo
    auto logo = new LogoWidget();

    // Buttons
    setupHeaderButtons();

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(10, 0, 6, 0);

    layout->addWidget(logo);
    layout->addStretch();
    layout->addWidget(m_btPasswordLock);
    layout->addWidget(m_btPasswordUnlock);
    layout->addWidget(m_headerButtons);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x26, 0x26, 0x26));
    palette.setColor(QPalette::WindowText, Qt::white);

    auto frame = ControlUtil::wrapToWidget(layout);
    frame->setAutoFillBackground(true);
    frame->setPalette(palette);

    // After the Password buttons are in the frame: a parentless widget shown is a window
    updatePasswordButtons();

    return frame;
}

void HomeWindow::setupHeaderButtons()
{
    // Password Unlock button
    setupPasswordButtons();

    // Header buttons
    m_headerButtons = new HeaderButtons();
}

void HomeWindow::setupPasswordButtons()
{
    m_btPasswordLock = ControlUtil::createToolButton(
            ":/icons/lock.png", [&] { windowManager()->resetCheckedPassword(); });

    m_btPasswordUnlock = ControlUtil::createToolButton(":/icons/lock_open.png");

    connect(
            m_btPasswordUnlock, &QToolButton::clicked, this,
            [&] { windowManager()->checkPassword(WindowHome); }, Qt::QueuedConnection);

    connect(ctrl(), &HomeController::passwordLockedChanged, this,
            &HomeWindow::updatePasswordButtons);
}

QLayout *HomeWindow::setupDialogButtons()
{
    auto layout = ControlUtil::createHLayout();
    layout->setContentsMargins(6, 4, 6, 4);

    for (int i = 0; i < HomeLinks::LinkCount; ++i) {
        const auto link = HomeLinks::Link(i);

        auto c = ControlUtil::createFlatToolButton(
                HomeLinks::iconPath(link), [=] { HomeLinks::open(link); });
        c->setToolTip(HomeLinks::path(link));

        if (!HomeLinks::isVisible(link)) {
            c->hide();
        }

        m_linkButtons.append(c);
        layout->addWidget(c);
    }

    layout->addStretch();

    return layout;
}

void HomeWindow::updatePasswordButtons()
{
    m_btPasswordLock->setVisible(settings()->hasPassword() && !ctrl()->passwordLocked());
    m_btPasswordUnlock->setVisible(ctrl()->passwordLocked());
}
