#include "policieswindow.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/tableview.h>
#include <manager/windowmanager.h>
#include <model/policylistmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "policiescontroller.h"
#include "policylistbox.h"

namespace {

constexpr int POLICIES_SPLIT_VERSION = 1;

}

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

void PoliciesWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setPolicyWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setPolicyWindowMaximized(m_stateWatcher->maximized());

    iniUser()->setPolicyWindowSplit(m_splitter->saveState());
    iniUser()->setPolicyWindowPresetSplit(m_presetSplitter->saveState());
    iniUser()->setPolicyWindowGlobalSplit(m_globalSplitter->saveState());
    iniUser()->setPolicyWindowSplitVersion(POLICIES_SPLIT_VERSION);

    confManager()->saveIniUser();
}

void PoliciesWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(800, 600), iniUser()->policyWindowGeometry(),
            iniUser()->policyWindowMaximized());

    if (iniUser()->policyWindowSplitVersion() == POLICIES_SPLIT_VERSION) {
        m_splitter->restoreState(iniUser()->policyWindowSplit());
        m_presetSplitter->restoreState(iniUser()->policyWindowPresetSplit());
        m_globalSplitter->restoreState(iniUser()->policyWindowGlobalSplit());
    }
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

    m_presetLibBox->label()->setText(tr("Library of preset rules:"));
    m_presetAppBox->label()->setText(tr("Preset rules for applications:"));
    m_globalPreBox->label()->setText(tr("Global rules, applied before application rules:"));
    m_globalPostBox->label()->setText(tr("Global rules, applied after application rules:"));

    m_presetLibBox->onRetranslateUi();
    m_presetAppBox->onRetranslateUi();
    m_globalPreBox->onRetranslateUi();
    m_globalPostBox->onRetranslateUi();

    this->setWindowTitle(tr("Policies"));
}

void PoliciesWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Column #1
    setupPresetSplitter();

    // Column #2
    setupGlobalSplitter();

    // Splitter
    m_splitter = new QSplitter();
    m_splitter->setHandleWidth(12);
    m_splitter->addWidget(m_presetSplitter);
    m_splitter->addWidget(m_globalSplitter);

    layout->addWidget(m_splitter);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(
            GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/traffic_lights.png"));

    // Size
    this->setMinimumSize(500, 400);
}

void PoliciesWindow::setupPresetSplitter()
{
    // Preset Lib Group Box
    setupPresetLibBox();

    // Preset App Group Box
    setupPresetAppBox();

    // Splitter
    m_presetSplitter = new QSplitter();
    m_presetSplitter->setHandleWidth(10);
    m_presetSplitter->setOrientation(Qt::Vertical);
    m_presetSplitter->addWidget(m_presetLibBox);
    m_presetSplitter->addWidget(m_presetAppBox);
}

void PoliciesWindow::setupPresetLibBox()
{
    m_presetLibBox = new PolicyListBox(Policy::TypePresetLibrary);
}

void PoliciesWindow::setupPresetAppBox()
{
    m_presetAppBox = new PolicyListBox(Policy::TypePresetApp);
}

void PoliciesWindow::setupGlobalSplitter()
{
    // Global Pre Group Box
    setupGlobalPreBox();

    // Global Post Group Box
    setupGlobalPostBox();

    // Splitter
    m_globalSplitter = new QSplitter();
    m_globalSplitter->setHandleWidth(10);
    m_globalSplitter->setOrientation(Qt::Vertical);
    m_globalSplitter->addWidget(m_globalPreBox);
    m_globalSplitter->addWidget(m_globalPostBox);
}

void PoliciesWindow::setupGlobalPreBox()
{
    m_globalPreBox = new PolicyListBox(Policy::TypeGlobalBeforeApp);
}

void PoliciesWindow::setupGlobalPostBox()
{
    m_globalPostBox = new PolicyListBox(Policy::TypeGlobalAfterApp);
}
