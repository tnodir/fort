#include "programalertwindow.h"

#include <QRadioButton>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <form/conn/appalertconnswindow.h>
#include <form/conn/appalertpendingconnswindow.h>
#include <fortglobal.h>
#include <stat/pendingconnmanager.h>
#include <user/iniuser.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "programeditcontroller.h"

using namespace Fort;

namespace {

// The oldest pending connection's Program, except the given one: it's blocked by the timeout first
App pendingConnApp(const App &exceptApp)
{
    auto confAppManager = Fort::confAppManager();

    const auto connApp = pendingConnManager()->oldestConnApp(exceptApp.appId, exceptApp.appPath);

    if (connApp.confAppId != 0) {
        return confAppManager->appById(connApp.confAppId);
    }

    if (!connApp.appPath.isEmpty()) {
        return confAppManager->appByPath(connApp.appPath);
    }

    return {};
}

App alertingApp()
{
    auto confAppManager = Fort::confAppManager();

    const qint64 appId = confAppManager->getAlertAppId();

    return confAppManager->appById(appId);
}

// The pending connections' Program first
App alertApp(const App &savedApp)
{
    // The saved Program's pending connections are deleted by their reauthorization or asked later
    const App app = pendingConnApp(savedApp);
    if (!app.appPath.isEmpty())
        return app;

    return alertingApp();
}

}

ProgramAlertWindow::ProgramAlertWindow(QWidget *parent) : ProgramEditDialog(parent)
{
    setupUi();

    setupFormWindow(iniUser(), IniUser::progAlertWindowGroup());

    initialize();

    connect(confAppManager(), &ConfAppManager::appDeleted, this, &ProgramAlertWindow::onAppDeleted);
}

bool ProgramAlertWindow::isAutoActive() const
{
    return iniUser().progAlertWindowAutoActive();
}

void ProgramAlertWindow::initialize()
{
    const auto app = alertApp(ctrl()->app());

    ProgramEditDialog::initialize(app);
}

void ProgramAlertWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setProgAlertWindowGeometry(stateWatcher()->geometry());
    iniUser.setProgAlertWindowMaximized(stateWatcher()->maximized());

    confManager()->saveIniUser();
}

void ProgramAlertWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(this, QSize(500, 400), iniUser.progAlertWindowGeometry(),
            iniUser.progAlertWindowMaximized());
}

void ProgramAlertWindow::closeOnSave()
{
    initialize();

    if (isNew()) {
        ProgramEditDialog::closeOnSave();
    }
}

AppConnsWindow *ProgramAlertWindow::createConnsWindow()
{
    return new AppAlertConnsWindow(ctrl()->app(), this);
}

AppPendingConnsWindow *ProgramAlertWindow::createPendingConnsWindow()
{
    return new AppAlertPendingConnsWindow(ctrl()->app(), this);
}

void ProgramAlertWindow::onAppDeleted(qint64 appId)
{
    if (appId == ctrl()->app().appId) {
        close(); // The alerted app was deleted, close the alert window
    }
}

void ProgramAlertWindow::retranslateWindowTitle()
{
    this->setWindowTitle(tr("Alert Program"));
}

void ProgramAlertWindow::setupUi()
{
    // Modality
    this->setWindowModality(Qt::NonModal);

    // Top Window
    this->setWindowFlag(Qt::WindowStaysOnTopHint, iniUser().progAlertWindowAlwaysOnTop());
    this->setAttribute(Qt::WA_ShowWithoutActivating, !isAutoActive());
}
