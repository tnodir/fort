#include "rulespage.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

#include <form/controls/tableview.h>
#include <model/policylistmodel.h>
#include <user/iniuser.h>

#include "rules/policylistbox.h"

namespace {

constexpr int RULES_SPLIT_VERSION = 1;

}

RulesPage::RulesPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void RulesPage::onSaveWindowState(IniUser *ini)
{
    ini->setPolicyWindowRulesSplit(m_splitter->saveState());
    ini->setPolicyWindowRulesPresetSplit(m_presetSplitter->saveState());
    ini->setPolicyWindowRulesGlobalSplit(m_globalSplitter->saveState());
    ini->setPolicyWindowRulesSplitVersion(RULES_SPLIT_VERSION);
}

void RulesPage::onRestoreWindowState(IniUser *ini)
{
    if (ini->policyWindowRulesSplitVersion() == RULES_SPLIT_VERSION) {
        m_splitter->restoreState(ini->policyWindowRulesSplit());
        m_presetSplitter->restoreState(ini->policyWindowRulesPresetSplit());
        m_globalSplitter->restoreState(ini->policyWindowRulesGlobalSplit());
    }
}

void RulesPage::onRetranslateUi()
{
    m_presetLibBox->label()->setText(tr("Library of preset rules:"));
    m_presetAppBox->label()->setText(tr("Preset rules for applications:"));
    m_globalPreBox->label()->setText(tr("Global rules, applied before application rules:"));
    m_globalPostBox->label()->setText(tr("Global rules, applied after application rules:"));

    m_presetLibBox->onRetranslateUi();
    m_presetAppBox->onRetranslateUi();
    m_globalPreBox->onRetranslateUi();
    m_globalPostBox->onRetranslateUi();
}

void RulesPage::setupUi()
{
    // Column #1
    setupPresetSplitter();

    // Column #2
    setupGlobalSplitter();

    // Splitter
    m_splitter = new QSplitter();
    m_splitter->setHandleWidth(12);
    m_splitter->addWidget(m_presetSplitter);
    m_splitter->addWidget(m_globalSplitter);

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addWidget(m_splitter);

    this->setLayout(layout);
}

void RulesPage::setupPresetSplitter()
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

void RulesPage::setupPresetLibBox()
{
    m_presetLibBox = new PolicyListBox(Policy::TypePresetLibrary);
}

void RulesPage::setupPresetAppBox()
{
    m_presetAppBox = new PolicyListBox(Policy::TypePresetApp);
}

void RulesPage::setupGlobalSplitter()
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

void RulesPage::setupGlobalPreBox()
{
    m_globalPreBox = new PolicyListBox(Policy::TypeGlobalBeforeApp);
}

void RulesPage::setupGlobalPostBox()
{
    m_globalPostBox = new PolicyListBox(Policy::TypeGlobalAfterApp);
}
