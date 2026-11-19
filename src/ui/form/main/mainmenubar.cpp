#include "mainmenubar.h"

#include <form/home/homelinks.h>
#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

#include "mainwindow.h"

using namespace Fort;

MainMenuBar::MainMenuBar(MainWindow *mainWindow) : QMenuBar(mainWindow), m_mainWindow(mainWindow)
{
    setupUi();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &MainMenuBar::retranslateUi);
    retranslateUi();
}

void MainMenuBar::retranslateUi()
{
    m_fileMenu->setTitle(tr("File"));
    m_actLock->setText(tr("Lock"));
    m_actUnlock->setText(tr("Unlock"));

    m_toolsMenu->setTitle(tr("Tools"));
    m_actReviewAlerts->setText(tr("Review Alerts"));

    m_windowMenu->setTitle(tr("Window"));
    m_actResetLayout->setText(tr("Reset Layout"));

    m_helpMenu->setTitle(tr("Help"));
    m_actHelp->setText(HomeLinks::text(HomeLinks::LinkHelp));
    for (int i = 0; i < m_linkActions.size(); ++i) {
        m_linkActions[i]->setText(HomeLinks::text(HomeLinks::Link(i)));
    }
    m_actAbout->setText(tr("About"));
}

void MainMenuBar::setupUi()
{
    setupFileMenu();
    setupToolsMenu();
    setupWindowMenu();
    setupHelpMenu();
}

void MainMenuBar::setupFileMenu()
{
    const auto trayIcon = windowManager()->trayIcon();

    m_fileMenu = addMenu(QString());

    m_fileMenu->addAction(trayIcon->homeAction());

    m_actLock = m_fileMenu->addAction(IconCache::icon(":/icons/lock.png"), QString());
    connect(m_actLock, &QAction::triggered, windowManager(), &WindowManager::resetCheckedPassword);

    m_actUnlock = m_fileMenu->addAction(IconCache::icon(":/icons/lock_open.png"), QString());
    connect(
            m_actUnlock, &QAction::triggered, this,
            [] { windowManager()->checkPassword(WindowMain); }, Qt::QueuedConnection);

    m_fileMenu->addSeparator();

    m_fileMenu->addAction(trayIcon->quitAction());

    connect(m_fileMenu, &QMenu::aboutToShow, this, &MainMenuBar::updateFileMenu);
}

void MainMenuBar::setupToolsMenu()
{
    m_toolsMenu = addMenu(QString());

    m_actReviewAlerts = m_toolsMenu->addAction(IconCache::icon(":/icons/error.png"), QString());
    connect(m_actReviewAlerts, &QAction::triggered, windowManager(),
            [] { windowManager()->showProgramAlertWindow(); });
}

void MainMenuBar::setupWindowMenu()
{
    m_windowMenu = addMenu(QString());

    m_actResetLayout =
            m_windowMenu->addAction(IconCache::icon(":/icons/arrow_refresh_small.png"), QString());
    connect(m_actResetLayout, &QAction::triggered, m_mainWindow, &MainWindow::resetDocksLayout);
}

void MainMenuBar::setupHelpMenu()
{
    m_helpMenu = addMenu(QString());
    m_helpMenu->setToolTipsVisible(true);

    m_actHelp = m_mainWindow->helpAction();
    m_actHelp->setIcon(IconCache::icon(HomeLinks::iconPath(HomeLinks::LinkHelp)));
    m_helpMenu->addAction(m_actHelp);

    m_helpMenu->addSeparator();

    // The Help link is opened by the Help action
    for (int i = 0; i < HomeLinks::LinkHelp; ++i) {
        const auto link = HomeLinks::Link(i);

        auto a = m_helpMenu->addAction(IconCache::icon(HomeLinks::iconPath(link)), {});
        a->setToolTip(HomeLinks::path(link));
        a->setVisible(HomeLinks::isVisible(link));

        connect(a, &QAction::triggered, this, [=] { HomeLinks::open(link); });

        m_linkActions.append(a);
    }

    m_helpMenu->addSeparator();

    m_actAbout = m_helpMenu->addAction(IconCache::icon(":/icons/fort.png"), QString());
    connect(m_actAbout, &QAction::triggered, windowManager(), &WindowManager::showHomeWindowAbout);
}

void MainMenuBar::updateFileMenu()
{
    const auto settings = Fort::settings();

    const bool passwordLocked = settings->isPasswordRequired();

    m_actLock->setVisible(settings->hasPassword() && !passwordLocked);
    m_actUnlock->setVisible(passwordLocked);
}
