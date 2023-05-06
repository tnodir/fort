#include "homewindow.h"

#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "homecontroller.h"

HomeWindow::HomeWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new HomeController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

ConfManager *HomeWindow::confManager() const
{
    return ctrl()->confManager();
}

IniUser *HomeWindow::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *HomeWindow::windowManager() const
{
    return ctrl()->windowManager();
}

void HomeWindow::saveWindowState()
{
    iniUser()->setServiceWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setServiceWindowMaximized(m_stateWatcher->maximized());

    confManager()->saveIniUser();
}

void HomeWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(600, 400), iniUser()->homeWindowGeometry(),
            iniUser()->homeWindowMaximized());
}

void HomeWindow::retranslateUi()
{
    this->unsetLocale();

    this->setWindowTitle(tr("My Fort"));
}

void HomeWindow::setupController()
{
    connect(ctrl(), &HomeController::retranslateUi, this, &HomeWindow::retranslateUi);

    retranslateUi();
}

void HomeWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void HomeWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}
