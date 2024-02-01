#include "programalertwindow.h"

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "programscontroller.h"

ProgramAlertWindow::ProgramAlertWindow(QWidget *parent) :
    ProgramEditDialog(new ProgramsController(/*this*/), parent),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();

    initialize();
}

void ProgramAlertWindow::initialize()
{
    const qint64 appId = confAppManager()->getAlertAppId();
    const auto appRow = appListModel()->appRowById(appId);

    ProgramEditDialog::initialize(appRow);
}

void ProgramAlertWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setProgAlertWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setProgAlertWindowMaximized(m_stateWatcher->maximized());

    confManager()->saveIniUser();
}

void ProgramAlertWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(500, 400), iniUser()->progAlertWindowGeometry(),
            iniUser()->progAlertWindowMaximized());
}

void ProgramAlertWindow::closeOnSave()
{
    initialize();

    if (isEmpty()) {
        ProgramEditDialog::closeOnSave();
    }
}

void ProgramAlertWindow::setupController()
{
    ctrl()->setParent(this); // can't set in ctor, because the widget isn't yet fully constructed
    ctrl()->initialize();
}

void ProgramAlertWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
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

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/error.png"));
}
