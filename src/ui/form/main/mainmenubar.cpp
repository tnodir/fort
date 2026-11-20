#include "mainmenubar.h"

#include <form/controls/applinks.h>
#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

#include "backupcontroller.h"
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

    m_backupMenu->setTitle(tr("Backup"));
    m_actExport->setText(tr("Export"));
    m_actImport->setText(tr("Import"));
    m_actImportApps->setText(tr("Import new programs"));

    m_toolsMenu->setTitle(tr("Tools"));
    m_actReviewAlerts->setText(tr("Review Alerts"));

    m_windowMenu->setTitle(tr("Window"));
    m_actInstallation->setText(tr("Installation"));
    m_actResetLayout->setText(tr("Reset Layout"));

    m_helpMenu->setTitle(tr("Help"));
    m_actHelp->setText(AppLinks::text(AppLinks::LinkHelp));
    for (int i = 0; i < m_linkActions.size(); ++i) {
        m_linkActions[i]->setText(AppLinks::text(AppLinks::Link(i)));
    }
    m_actCheckUpdates->setText(tr("Check for Updates"));
    m_actAbout->setText(tr("About"));
}

void MainMenuBar::setLocked(bool locked)
{
    m_backupMenu->setEnabled(!locked);
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

    m_fileMenu->addAction(m_mainWindow->lockAction());
    m_fileMenu->addAction(m_mainWindow->unlockAction());

    m_fileMenu->addSeparator();

    setupBackupMenu();

    m_fileMenu->addSeparator();

    m_fileMenu->addAction(trayIcon->quitAction());
}

void MainMenuBar::setupBackupMenu()
{
    m_backupMenu = m_fileMenu->addMenu(IconCache::icon(":/icons/save_as.png"), QString());

    m_actExport = m_backupMenu->addAction(IconCache::icon(":/icons/disk.png"), QString());
    m_actImport = m_backupMenu->addAction(IconCache::icon(":/icons/folder.png"), QString());
    m_actImportApps =
            m_backupMenu->addAction(IconCache::icon(":/icons/application.png"), QString());

    auto ctrl = new BackupController(this);

    connect(m_actExport, &QAction::triggered, ctrl, &BackupController::exportBackup);
    connect(m_actImport, &QAction::triggered, ctrl, &BackupController::confirmImportBackup);
    connect(m_actImportApps, &QAction::triggered, ctrl, &BackupController::confirmImportAppsBackup);
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

    m_actInstallation =
            m_windowMenu->addAction(IconCache::icon(":/icons/server_components.png"), QString());
    connect(m_actInstallation, &QAction::triggered, windowManager(),
            &WindowManager::showInstallationWindow);

    m_windowMenu->addSeparator();

    m_actResetLayout =
            m_windowMenu->addAction(IconCache::icon(":/icons/arrow_refresh_small.png"), QString());
    connect(m_actResetLayout, &QAction::triggered, m_mainWindow, &MainWindow::resetDocksLayout);
}

void MainMenuBar::setupHelpMenu()
{
    m_helpMenu = addMenu(QString());
    m_helpMenu->setToolTipsVisible(true);

    m_actHelp = m_mainWindow->helpAction();
    m_actHelp->setIcon(IconCache::icon(AppLinks::iconPath(AppLinks::LinkHelp)));
    m_helpMenu->addAction(m_actHelp);

    m_helpMenu->addSeparator();

    // The Help link is opened by the Help action
    for (int i = 0; i < AppLinks::LinkHelp; ++i) {
        const auto link = AppLinks::Link(i);

        auto a = m_helpMenu->addAction(IconCache::icon(AppLinks::iconPath(link)), {});
        a->setToolTip(AppLinks::path(link));
        a->setVisible(AppLinks::isVisible(link));

        connect(a, &QAction::triggered, this, [=] { AppLinks::open(link); });

        m_linkActions.append(a);
    }

    m_helpMenu->addSeparator();

    m_actCheckUpdates = m_helpMenu->addAction(IconCache::icon(":/icons/download.png"), QString());
    connect(m_actCheckUpdates, &QAction::triggered, windowManager(),
            &WindowManager::showUpdatesWindow);

    m_actAbout = m_helpMenu->addAction(IconCache::icon(":/icons/information.png"), QString());
    connect(m_actAbout, &QAction::triggered, windowManager(), &WindowManager::showAboutBox);
}
