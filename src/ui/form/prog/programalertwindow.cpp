#include "programalertwindow.h"

#include <QRadioButton>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <model/applistmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "programeditcontroller.h"

ProgramAlertWindow::ProgramAlertWindow(QWidget *parent) :
    ProgramEditDialog(new ProgramEditController(/*this*/), parent)
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::progAlertWindowGroup());

    initialize();
}

ConfAppManager *ProgramAlertWindow::confAppManager() const
{
    return ctrl()->confAppManager();
}

ConfManager *ProgramAlertWindow::confManager() const
{
    return ctrl()->confManager();
}

IniUser *ProgramAlertWindow::iniUser() const
{
    return ctrl()->iniUser();
}

bool ProgramAlertWindow::isAutoActive() const
{
    return iniUser()->progAlertWindowAutoActive();
}

void ProgramAlertWindow::initialize()
{
    const qint64 appId = confAppManager()->getAlertAppId();
    const auto app = confAppManager()->appById(appId);

    ProgramEditDialog::initialize(app);
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

void ProgramAlertWindow::retranslateWindowTitle()
{
    this->setWindowTitle(tr("Alert Program"));
}

void ProgramAlertWindow::setupController()
{
    ctrl()->setParent(this); // can't set in ctor, because the widget isn't yet fully constructed
}

void ProgramAlertWindow::setupUi()
{
    // Modality
    this->setWindowModality(Qt::NonModal);

    // Top Window
    this->setWindowFlag(Qt::WindowStaysOnTopHint, iniUser()->progAlertWindowAlwaysOnTop());
    this->setAttribute(Qt::WA_ShowWithoutActivating, !isAutoActive());
}
