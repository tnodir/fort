#include "rulespage.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QVBoxLayout>

#include <form/controls/tableview.h>
#include <model/policylistmodel.h>
#include <user/iniuser.h>

RulesPage::RulesPage(OptionsController *ctrl, QWidget *parent) :
    OptBasePage(ctrl, parent),
    m_presetLibListModel(new PolicyListModel(PolicyListModel::PolicyListPresetLibrary, this)),
    m_presetAppListModel(new PolicyListModel(PolicyListModel::PolicyListPresetApp, this)),
    m_globalPreListModel(new PolicyListModel(PolicyListModel::PolicyListGlobalBeforeApp, this)),
    m_globalPostListModel(new PolicyListModel(PolicyListModel::PolicyListGlobalAfterApp, this))
{
    setupUi();

    presetAppListModel()->initialize();
    presetLibListModel()->initialize();
    globalPreListModel()->initialize();
    globalPostListModel()->initialize();
}

void RulesPage::onSaveWindowState(IniUser *ini)
{
    ini->setOptWindowRulesSplit(m_splitter->saveState());
    ini->setOptWindowRulesPresetSplit(m_presetSplitter->saveState());
    ini->setOptWindowRulesGlobalSplit(m_globalSplitter->saveState());
}

void RulesPage::onRestoreWindowState(IniUser *ini)
{
    m_splitter->restoreState(ini->optWindowRulesSplit());
    m_presetSplitter->restoreState(ini->optWindowRulesPresetSplit());
    m_globalSplitter->restoreState(ini->optWindowRulesGlobalSplit());
}

void RulesPage::onRetranslateUi()
{
    m_gbPresetLib->setTitle(tr("Library of preset rules:"));
    m_gbPresetApp->setTitle(tr("Preset rules for applications:"));
    m_gbGlobalPre->setTitle(tr("Global rules, applied before application rules:"));
    m_gbGlobalPost->setTitle(tr("Global rules, applied after application rules:"));
}

void RulesPage::setupUi()
{
    // Column #1
    setupPresetSplitter();

    // Column #2
    setupGlobalSplitter();

    // Splitter
    m_splitter = new QSplitter();
    m_splitter->setHandleWidth(20);
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
    m_presetSplitter->addWidget(m_gbPresetLib);
    m_presetSplitter->addWidget(m_gbPresetApp);
}

void RulesPage::setupPresetLibBox()
{
    setupPresetLibView();

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_presetLibListView);

    m_gbPresetLib = new QGroupBox(this);
    m_gbPresetLib->setFlat(true);
    m_gbPresetLib->setLayout(layout);
}

void RulesPage::setupPresetLibView()
{
    m_presetLibListView = new TableView();
    m_presetLibListView->setAlternatingRowColors(true);
    m_presetLibListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_presetLibListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_presetLibListView->horizontalHeader()->setVisible(false);

    m_presetLibListView->setModel(presetLibListModel());

    // m_presetLibListView->setMenu(m_btEdit->menu());
}

void RulesPage::setupPresetAppBox()
{
    setupPresetAppView();

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_presetAppListView);

    m_gbPresetApp = new QGroupBox(this);
    m_gbPresetApp->setFlat(true);
    m_gbPresetApp->setLayout(layout);
}

void RulesPage::setupPresetAppView()
{
    m_presetAppListView = new TableView();
    m_presetAppListView->setAlternatingRowColors(true);
    m_presetAppListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_presetAppListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_presetAppListView->horizontalHeader()->setVisible(false);

    m_presetAppListView->setModel(presetAppListModel());
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
    m_globalSplitter->addWidget(m_gbGlobalPre);
    m_globalSplitter->addWidget(m_gbGlobalPost);
}

void RulesPage::setupGlobalPreBox()
{
    setupGlobalPreView();

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_globalPreListView);

    m_gbGlobalPre = new QGroupBox(this);
    m_gbGlobalPre->setFlat(true);
    m_gbGlobalPre->setLayout(layout);
}

void RulesPage::setupGlobalPreView()
{
    m_globalPreListView = new TableView();
    m_globalPreListView->setAlternatingRowColors(true);
    m_globalPreListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_globalPreListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_globalPreListView->horizontalHeader()->setVisible(false);

    m_globalPreListView->setModel(globalPreListModel());
}

void RulesPage::setupGlobalPostBox()
{
    setupGlobalPostView();

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_globalPostListView);

    m_gbGlobalPost = new QGroupBox(this);
    m_gbGlobalPost->setFlat(true);
    m_gbGlobalPost->setLayout(layout);
}

void RulesPage::setupGlobalPostView()
{
    m_globalPostListView = new TableView();
    m_globalPostListView->setAlternatingRowColors(true);
    m_globalPostListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_globalPostListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_globalPostListView->horizontalHeader()->setVisible(false);

    m_globalPostListView->setModel(globalPostListModel());
}
