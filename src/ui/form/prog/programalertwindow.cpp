#include "programalertwindow.h"

#include <QRadioButton>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <fortglobal.h>
#include <user/iniuser.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "programeditcontroller.h"

using namespace Fort;

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
    const qint64 appId = confAppManager()->getAlertAppId();
    const auto app = confAppManager()->appById(appId);

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
