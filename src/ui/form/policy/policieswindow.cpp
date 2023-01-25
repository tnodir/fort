#include "policieswindow.h"

#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "policiescontroller.h"

PoliciesWindow::PoliciesWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new PoliciesController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

ConfManager *PoliciesWindow::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *PoliciesWindow::conf() const
{
    return ctrl()->conf();
}

IniOptions *PoliciesWindow::ini() const
{
    return ctrl()->ini();
}

IniUser *PoliciesWindow::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *PoliciesWindow::windowManager() const
{
    return ctrl()->windowManager();
}

void PoliciesWindow::saveWindowState()
{
    iniUser()->setPolicyWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setPolicyWindowMaximized(m_stateWatcher->maximized());

    confManager()->saveIniUser();
}

void PoliciesWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(1024, 768), iniUser()->policyWindowGeometry(),
            iniUser()->policyWindowMaximized());
}

void PoliciesWindow::setupController()
{
    ctrl()->initialize();

    connect(ctrl(), &PoliciesController::retranslateUi, this, &PoliciesWindow::retranslateUi);

    retranslateUi();
}

void PoliciesWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void PoliciesWindow::retranslateUi()
{
    this->unsetLocale();

    this->setWindowTitle(tr("Policies"));
}

void PoliciesWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/node-tree.png"));

    // Size
    this->setMinimumSize(500, 400);
}
