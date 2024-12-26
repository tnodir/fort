#include "programalertwindow.h"

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "programscontroller.h"

ProgramAlertWindow::ProgramAlertWindow(QWidget *parent) :
    ProgramEditDialog(new ProgramsController(/*this*/), parent)
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::progAlertWindowGroup());

    initialize();
}

bool ProgramAlertWindow::isAutoActive() const
{
    return iniUser()->progAlertWindowAutoActive();
}

void ProgramAlertWindow::initialize()
{
    const qint64 appId = confAppManager()->getAlertAppId();
    const auto appRow = appListModel()->appRowById(appId);

    ProgramEditDialog::initialize(appRow);
}

void ProgramAlertWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setProgAlertWindowGeometry(stateWatcher()->geometry());
    iniUser()->setProgAlertWindowMaximized(stateWatcher()->maximized());

    confManager()->saveIniUser();
}

void ProgramAlertWindow::restoreWindowState()
{
    stateWatcher()->restore(this, QSize(500, 400), iniUser()->progAlertWindowGeometry(),
            iniUser()->progAlertWindowMaximized());
}

void ProgramAlertWindow::closeOnSave()
{
    initialize();

    if (isNew()) {
        ProgramEditDialog::closeOnSave();
    }
}

void ProgramAlertWindow::setupController()
{
    ctrl()->setParent(this); // can't set in ctor, because the widget isn't yet fully constructed
    ctrl()->initialize();
}

void ProgramAlertWindow::retranslateWindowTitle()
{
    this->setWindowTitle(tr("Alert Program"));
}

void ProgramAlertWindow::setupUi()
{
    // Advanced Mode
    setAdvancedMode(false);

    // Modality
    this->setWindowModality(Qt::NonModal);

    // Top Window
    this->setWindowFlag(Qt::WindowStaysOnTopHint, iniUser()->progAlertWindowAlwaysOnTop());
    this->setAttribute(Qt::WA_ShowWithoutActivating, !isAutoActive());
}
