#include "statisticswindow.h"

#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "pages/statmainpage.h"
#include "statisticscontroller.h"

StatisticsWindow::StatisticsWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new StatisticsController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::statWindowGroup());
}

ConfManager *StatisticsWindow::confManager() const
{
    return ctrl()->confManager();
}

IniUser *StatisticsWindow::iniUser() const
{
    return ctrl()->iniUser();
}

void StatisticsWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setStatWindowGeometry(stateWatcher()->geometry());
    iniUser()->setStatWindowMaximized(stateWatcher()->maximized());

    emit ctrl()->afterSaveWindowState(iniUser());

    confManager()->saveIniUser();
}

void StatisticsWindow::restoreWindowState()
{
    stateWatcher()->restore(this, QSize(1024, 768), iniUser()->statWindowGeometry(),
            iniUser()->statWindowMaximized());

    emit ctrl()->afterRestoreWindowState(iniUser());
}

void StatisticsWindow::setupController()
{
    connect(ctrl(), &StatisticsController::retranslateUi, this, &StatisticsWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void StatisticsWindow::retranslateUi()
{
    this->unsetLocale();

    this->setWindowTitle(tr("Statistics"));
}

void StatisticsWindow::setupUi()
{
    auto mainPage = new StatMainPage(ctrl());

    auto layout = ControlUtil::createVLayout();
    layout->addWidget(mainPage);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}
