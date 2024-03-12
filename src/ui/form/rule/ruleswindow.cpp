#include "ruleswindow.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/treeview.h>
#include <form/dialog/dialogutil.h>
#include <manager/windowmanager.h>
#include <model/rulelistmodel.h>
#include <user/iniuser.h>
#include <util/conf/confutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "ruleeditdialog.h"
#include "rulescontroller.h"

namespace {

constexpr int RULES_HEADER_VERSION = 2;

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

RuleListModel *RulesWindow::ruleListModel() const
{
    return ctrl()->ruleListModel();
}

void RulesWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setRuleWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setRuleWindowMaximized(m_stateWatcher->maximized());

    auto header = m_ruleListView->header();
    iniUser()->setRulesHeader(header->saveState());
    iniUser()->setRulesHeaderVersion(RULES_HEADER_VERSION);

    confManager()->saveIniUser();
}

void RulesWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(500, 600), iniUser()->ruleWindowGeometry(),
            iniUser()->ruleWindowMaximized());

    if (iniUser()->rulesHeaderVersion() == RULES_HEADER_VERSION) {
        auto header = m_ruleListView->header();
        header->restoreState(iniUser()->rulesHeader());
    }
}

void RulesWindow::setupController()
{
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

    m_btEdit->setText(tr("Edit"));
    m_actAddRule->setText(tr("Add"));
    m_actEditRule->setText(tr("Edit"));
    m_actRemoveRule->setText(tr("Remove"));
    m_editSearch->setPlaceholderText(tr("Search"));

    ruleListModel()->refresh();

    this->setWindowTitle(tr("Rules"));
}

void RulesWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Tree
    setupTreeRules();
    setupTreeRulesHeader();

    // Actions on rules tree's current changed
    setupTreeRulesChanged();

    // Actions on rule list model's changed
    setupRuleListModelChanged();

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addLayout(header);
    layout->addWidget(m_ruleListView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/script.png"));

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *RulesWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddRule = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddRule->setShortcut(Qt::Key_Plus);

    m_actEditRule = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditRule->setShortcut(Qt::Key_Enter);

    m_actRemoveRule = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveRule->setShortcut(Qt::Key_Delete);

    connect(m_actAddRule, &QAction::triggered, this, &RulesWindow::addNewRule);
    connect(m_actEditRule, &QAction::triggered, this, &RulesWindow::editSelectedRule);
    connect(m_actRemoveRule, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { deleteSelectedRule(); }, tr("Are you sure to remove selected rule?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Search field
    setupEditSearch();

    // Menu button
    m_btMenu = windowManager()->createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btEdit, ControlUtil::createVSeparator(),
            m_editSearch, /*stretch*/ nullptr, m_btMenu });

    return layout;
}

void RulesWindow::setupEditSearch()
{
    m_editSearch = ControlUtil::createLineEdit(
            QString(), [&](const QString &text) { ruleListModel()->setFtsFilter(text); });
    m_editSearch->setClearButtonEnabled(true);
    m_editSearch->setMaxLength(200);
    m_editSearch->setMinimumWidth(100);
    m_editSearch->setMaximumWidth(200);

    connect(this, &RulesWindow::aboutToShow, m_editSearch, qOverload<>(&QWidget::setFocus));
}

void RulesWindow::setupTreeRules()
{
    m_ruleListView = new TreeView();
    m_ruleListView->setAlternatingRowColors(true);
    m_ruleListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ruleListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_ruleListView->setupItemDelegate();
    m_ruleListView->setModel(ruleListModel());

    m_ruleListView->expandAll();

    m_ruleListView->setMenu(m_btEdit->menu());

    connect(m_ruleListView, &TreeView::activated, m_actEditRule, &QAction::trigger);
}

void RulesWindow::setupTreeRulesHeader()
{
    auto header = m_ruleListView->header();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setStretchLastSection(true);

    header->resizeSection(0, 360);
}

void RulesWindow::setupTreeRulesChanged()
{
    const auto refreshTreeRulesChanged = [&] {
        const auto ruleIndex = ruleListCurrentIndex();
        const bool ruleSelected = !RuleListModel::isIndexRoot(ruleIndex);
        m_actEditRule->setEnabled(ruleSelected);
        m_actRemoveRule->setEnabled(ruleSelected);
    };

    refreshTreeRulesChanged();

    connect(m_ruleListView, &TreeView::currentIndexChanged, this, refreshTreeRulesChanged);
}

void RulesWindow::setupRuleListModelChanged()
{
    const auto refreshAddRule = [&] {
        m_actAddRule->setEnabled(ruleListModel()->rowCount() < ConfUtil::ruleMaxCount());
    };

    refreshAddRule();

    connect(ruleListModel(), &RuleListModel::modelReset, this, refreshAddRule);
    connect(ruleListModel(), &RuleListModel::rowsRemoved, this, refreshAddRule);
}

void RulesWindow::addNewRule()
{
    const auto ruleIndex = ruleListCurrentIndex();

    RuleRow ruleRow;
    ruleRow.ruleType = RuleListModel::indexRuleType(ruleIndex);

    openRuleEditForm(ruleRow);
}

void RulesWindow::editSelectedRule()
{
    const auto ruleIndex = ruleListCurrentIndex();
    if (RuleListModel::isIndexRoot(ruleIndex))
        return;

    const RuleRow ruleRow = ruleListModel()->ruleRowAt(ruleIndex);

    openRuleEditForm(ruleRow);
}

void RulesWindow::openRuleEditForm(const RuleRow &ruleRow)
{
    if (!m_formRuleEdit) {
        m_formRuleEdit = new RuleEditDialog(ctrl(), this);
    }

    m_formRuleEdit->initialize(ruleRow);

    WidgetWindow::showWidget(m_formRuleEdit);
}

void RulesWindow::deleteSelectedRule()
{
    const auto ruleIndex = ruleListCurrentIndex();
    if (RuleListModel::isIndexRoot(ruleIndex))
        return;

    const auto ruleRow = ruleListModel()->ruleRowAt(ruleIndex);
    if (ruleRow.isNull())
        return;

    ctrl()->deleteRule(ruleRow.ruleId);
}

QModelIndex RulesWindow::ruleListCurrentIndex() const
{
    return m_ruleListView->currentIndex();
}
