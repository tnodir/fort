#include "ruleswindow.h"

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

#include "rulescontroller.h"
#include "rulelistbox.h"

namespace {

constexpr int POLICIES_SPLIT_VERSION = 1;

}

RulesWindow::RulesWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new RulesController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

ConfManager *RulesWindow::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *RulesWindow::conf() const
{
    return ctrl()->conf();
}

IniOptions *RulesWindow::ini() const
{
    return ctrl()->ini();
}

IniUser *RulesWindow::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *RulesWindow::windowManager() const
{
    return ctrl()->windowManager();
}

void RulesWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setRuleWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setRuleWindowMaximized(m_stateWatcher->maximized());

    iniUser()->setRuleWindowSplit(m_splitter->saveState());
    iniUser()->setRuleWindowPresetSplit(m_presetSplitter->saveState());
    iniUser()->setRuleWindowGlobalSplit(m_globalSplitter->saveState());
    iniUser()->setRuleWindowSplitVersion(POLICIES_SPLIT_VERSION);

    confManager()->saveIniUser();
}

void RulesWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(800, 600), iniUser()->ruleWindowGeometry(),
            iniUser()->ruleWindowMaximized());

    if (iniUser()->ruleWindowSplitVersion() == POLICIES_SPLIT_VERSION) {
        m_splitter->restoreState(iniUser()->ruleWindowSplit());
        m_presetSplitter->restoreState(iniUser()->ruleWindowPresetSplit());
        m_globalSplitter->restoreState(iniUser()->ruleWindowGlobalSplit());
    }
}

void RulesWindow::setupController()
{
    ctrl()->initialize();

    connect(ctrl(), &RulesController::retranslateUi, this, &RulesWindow::retranslateUi);

    retranslateUi();
}

void RulesWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void RulesWindow::retranslateUi()
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

void RulesWindow::setupUi()
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
            GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/checklist.png"));

    // Size
    this->setMinimumSize(500, 400);
}

void RulesWindow::setupPresetSplitter()
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

void RulesWindow::setupPresetLibBox()
{
    m_presetLibBox = new RuleListBox(Policy::TypePresetLibrary);
}

void RulesWindow::setupPresetAppBox()
{
    m_presetAppBox = new RuleListBox(Policy::TypePresetApp);
}

void RulesWindow::setupGlobalSplitter()
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

void RulesWindow::setupGlobalPreBox()
{
    m_globalPreBox = new RuleListBox(Policy::TypeGlobalBeforeApp);
}

void RulesWindow::setupGlobalPostBox()
{
    m_globalPostBox = new RuleListBox(Policy::TypeGlobalAfterApp);
}
