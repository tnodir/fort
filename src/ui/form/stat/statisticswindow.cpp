#include "statisticswindow.h"

#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "pages/statmainpage.h"
#include "statisticscontroller.h"

StatisticsWindow::StatisticsWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new StatisticsController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
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
    iniUser()->setStatWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setStatWindowMaximized(m_stateWatcher->maximized());

    emit ctrl()->afterSaveWindowState(iniUser());

    confManager()->saveIniUser();
}

void StatisticsWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(1024, 768), iniUser()->statWindowGeometry(),
            iniUser()->statWindowMaximized());

    emit ctrl()->afterRestoreWindowState(iniUser());
}

void StatisticsWindow::setupController()
{
    connect(ctrl(), &StatisticsController::retranslateUi, this, &StatisticsWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void StatisticsWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void StatisticsWindow::retranslateUi()
{
    this->unsetLocale();

    this->setWindowTitle(tr("Statistics"));
}

void StatisticsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    auto mainPage = new StatMainPage(ctrl());
    layout->addWidget(mainPage);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/chart_bar.png"));

    // Size
    this->setMinimumSize(500, 400);
}
