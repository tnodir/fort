#include "ruleswindow.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <bit>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/treeview.h>
#include <form/dialog/dialogutil.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/rulelistmodel.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "ruleeditdialog.h"
#include "rulescontroller.h"

using namespace Fort;

namespace {

inline constexpr int RULES_HEADER_VERSION = 5;

}

RulesWindow::RulesWindow(quint8 selectRuleTypes, QWidget *parent, Qt::WindowFlags f) :
    FormWindow(parent, f), m_selectRuleTypes(selectRuleTypes), m_ctrl(new RulesController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::ruleWindowGroup());
}

RuleListModel *RulesWindow::ruleListModel() const
{
    return ctrl()->ruleListModel();
}

void RulesWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setRuleWindowGeometry(stateWatcher()->geometry());
    iniUser.setRuleWindowMaximized(stateWatcher()->maximized());

    auto header = m_ruleListView->header();
    iniUser.setRulesHeader(header->saveState());
    iniUser.setRulesHeaderVersion(RULES_HEADER_VERSION);

    // The Select Rule dialog expands its rule types always
    if (!isOpenSelectRule()) {
        iniUser.setRulesExpanded(m_expandedRuleTypes);
    }

    confManager()->saveIniUser();
}

void RulesWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(
            this, QSize(500, 600), iniUser.ruleWindowGeometry(), iniUser.ruleWindowMaximized());

    if (iniUser.rulesHeaderVersion() == RULES_HEADER_VERSION) {
        auto header = m_ruleListView->header();
        header->restoreState(iniUser.rulesHeader());
    }

    m_expandedRuleTypes = iniUser.rulesExpanded();

    expandTreeRules();
}

void RulesWindow::setupController()
{
    connect(ctrl(), &RulesController::retranslateUi, this, &RulesWindow::retranslateUi);

    retranslateUi();
}

void RulesWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddRule->setText(tr("Add"));
    m_actEditRule->setText(tr("Edit"));
    m_actDuplicateRule->setText(tr("Duplicate"));
    m_actRemoveRule->setText(tr("Remove"));
    m_editSearch->setPlaceholderText(tr("Search") + " /");

    ruleListModel()->refresh();

    if (isOpenSelectRule()) {
        m_btOk->setText(tr("OK"));
        m_btCancel->setText(tr("Cancel"));
    }

    this->setWindowTitle(isOpenSelectRule() ? tr("Select Rule") : tr("Rules"));
}

void RulesWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Tree
    setupTreeRules();
    setupTreeRulesHeader();

    // Actions on rules tree's expanded/collapsed changed
    setupTreeRulesExpandingChanged();

    // Actions on rules tree's current changed
    setupTreeRulesChanged();

    // Actions on rule list model's changed/reset
    setupRuleListModelReset();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_ruleListView, 1);

    // OK/Cancel
    if (isOpenSelectRule()) {
        auto buttonsLayout = setupButtons();
        layout->addWidget(ControlUtil::createHSeparator());
        layout->addLayout(buttonsLayout);
    }

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *RulesWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddRule = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddRule->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    m_actEditRule = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditRule->setShortcut(Qt::Key_Enter);

    m_actDuplicateRule = editMenu->addAction(IconCache::icon(":/icons/page_copy.png"), QString());

    m_actRemoveRule = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveRule->setShortcut(Qt::Key_Delete);

    connect(m_actAddRule, &QAction::triggered, this, &RulesWindow::addNewRule);
    connect(m_actEditRule, &QAction::triggered, this, &RulesWindow::editSelectedRule);
    connect(m_actDuplicateRule, &QAction::triggered, this, &RulesWindow::duplicateSelectedRule);
    connect(m_actRemoveRule, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { deleteSelectedRule(); }, tr("Are you sure to remove selected rule?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Search field
    setupEditSearch();

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Statistics button
    m_btStatistics = ControlUtil::createStatisticsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btEdit, ControlUtil::createVSeparator(),
            m_editSearch, /*stretch*/ nullptr, m_btOptions, m_btStatistics, m_btMenu });

    return layout;
}

void RulesWindow::setupEditSearch()
{
    m_editSearch = ControlUtil::createLineEdit(
            QString(), [&](const QString &text) { ruleListModel()->setTextFilter(text); });
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
    m_ruleListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_ruleListView->setupItemDelegate();
    m_ruleListView->setModel(ruleListModel());

    m_ruleListView->setMenu(m_btEdit->menu());

    connect(m_ruleListView, &TreeView::activated, this, [&](const QModelIndex &index) {
        if (isOpenSelectRule()) {
            emitRuleSelected(index);
        } else {
            m_actEditRule->trigger();
        }
    });
}

void RulesWindow::setupTreeRulesHeader()
{
    auto header = m_ruleListView->header();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(3, QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(0, 300);
    header->resizeSection(1, 30);
    header->resizeSection(2, 30);
    header->resizeSection(3, 130);
}

void RulesWindow::setupTreeRulesExpandingChanged()
{
    const auto onExpandingChanged = [&](const QModelIndex &index) {
        const bool isExpanded = m_ruleListView->isExpanded(index);
        const auto ruleType = RuleListModel::indexRuleType(index);
        const auto typeBit = Rule::ruleTypeBit(ruleType);

        m_expandedRuleTypes =
                isExpanded ? (m_expandedRuleTypes | typeBit) : (m_expandedRuleTypes & ~typeBit);
    };

    connect(m_ruleListView, &TreeView::expanded, this, onExpandingChanged);
    connect(m_ruleListView, &TreeView::collapsed, this, onExpandingChanged);
}

void RulesWindow::setupTreeRulesChanged()
{
    const auto refreshTreeRulesChanged = [&] {
        const auto ruleIndex = ruleListCurrentIndex();
        const bool isRuleSelected = RuleListModel::isIndexRule(ruleIndex);
        m_actEditRule->setEnabled(isRuleSelected);
        m_actDuplicateRule->setEnabled(isRuleSelected);
        m_actRemoveRule->setEnabled(isRuleSelected);
    };

    refreshTreeRulesChanged();

    connect(m_ruleListView, &TreeView::currentIndexChanged, this, refreshTreeRulesChanged);
}

void RulesWindow::setupRuleListModelReset()
{
    expandTreeRules();

    connect(ruleListModel(), &RuleListModel::modelReset, this, &RulesWindow::expandTreeRules);
}

QLayout *RulesWindow::setupButtons()
{
    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] { emitRuleSelected(); });
    m_btOk->setDefault(true);

    // Cancel
    m_btCancel = new QPushButton();
    connect(m_btCancel, &QAbstractButton::clicked, this, &QWidget::close);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btOk, 1, Qt::AlignRight);
    layout->addWidget(m_btCancel);

    return layout;
}

void RulesWindow::expandTreeRules()
{
    for (int i = 0; i < Rule::RuleTypeCount; ++i) {
        const auto ruleType = Rule::RuleType(i);
        const auto index = ruleListModel()->indexRoot(ruleType);

        m_ruleListView->setRowHidden(index.row(), index.parent(), !isRuleTypeShown(ruleType));
        m_ruleListView->setExpanded(index, isRuleTypeExpanded(ruleType));
    }
}

bool RulesWindow::isRuleTypeShown(Rule::RuleType ruleType) const
{
    return !isOpenSelectRule() || isSelectRuleType(ruleType);
}

bool RulesWindow::isRuleTypeExpanded(Rule::RuleType ruleType) const
{
    if (isOpenSelectRule()) {
        return isSelectRuleType(ruleType);
    }

    return (m_expandedRuleTypes & Rule::ruleTypeBit(ruleType)) != 0;
}

bool RulesWindow::isSelectRuleType(Rule::RuleType ruleType) const
{
    return (selectRuleTypes() & Rule::ruleTypeBit(ruleType)) != 0;
}

Rule::RuleType RulesWindow::newRuleType() const
{
    const auto ruleType = RuleListModel::indexRuleType(ruleListCurrentIndex());

    if (isRuleTypeShown(ruleType))
        return ruleType;

    return Rule::RuleType(std::countr_zero(selectRuleTypes())); // the first selectable one
}

bool RulesWindow::emitRuleSelected()
{
    return emitRuleSelected(ruleListCurrentIndex());
}

bool RulesWindow::emitRuleSelected(const QModelIndex &index)
{
    if (!RuleListModel::isIndexRule(index))
        return false;

    const auto &ruleRow = ruleListModel()->ruleRowAt(index);
    if (ruleRow.isNull())
        return false;

    emit ruleSelected(ruleRow);

    this->close();

    return true;
}

void RulesWindow::addNewRule()
{
    RuleRow ruleRow;
    ruleRow.ruleType = newRuleType();

    openRuleEditForm(ruleRow);
}

void RulesWindow::editSelectedRule()
{
    const RuleRow *ruleRow = selectedRuleRow();
    if (!ruleRow)
        return;

    openRuleEditForm(*ruleRow);
}

void RulesWindow::duplicateSelectedRule()
{
    const RuleRow *ruleRow = selectedRuleRow();
    if (!ruleRow)
        return;

    ruleEditForm()->initializeDuplicate(*ruleRow);

    DialogUtil::showDialog(m_formRuleEdit);
}

void RulesWindow::openRuleEditForm(const RuleRow &ruleRow)
{
    ruleEditForm()->initialize(ruleRow);

    DialogUtil::showDialog(m_formRuleEdit);
}

RuleEditDialog *RulesWindow::ruleEditForm()
{
    if (!m_formRuleEdit) {
        m_formRuleEdit = new RuleEditDialog(ctrl(), this);
    }

    return m_formRuleEdit;
}

void RulesWindow::deleteSelectedRule()
{
    const RuleRow *ruleRow = selectedRuleRow();
    if (!ruleRow)
        return;

    ctrl()->deleteRule(ruleRow->ruleId);
}

const RuleRow *RulesWindow::selectedRuleRow() const
{
    const auto ruleIndex = ruleListCurrentIndex();
    if (!RuleListModel::isIndexRule(ruleIndex))
        return nullptr;

    const auto &ruleRow = ruleListModel()->ruleRowAt(ruleIndex);

    return ruleRow.isNull() ? nullptr : &ruleRow;
}

QModelIndex RulesWindow::ruleListCurrentIndex() const
{
    return m_ruleListView->currentIndex();
}

RulesWindow *RulesWindow::showRulesDialog(quint8 selectRuleTypes, QWidget *parent)
{
    auto w = new RulesWindow(selectRuleTypes, parent, Qt::Dialog);
    ControlUtil::deleteOnClose(w);

    w->setWindowModality(Qt::WindowModal);

    w->showWindow();

    return w;
}

RuleEditDialog *RulesWindow::showRuleEditDialog(int ruleId, QWidget *parent)
{
    auto ctrl = new RulesController();

    auto w = new RuleEditDialog(ctrl, parent);
    ControlUtil::deleteOnClose(w);

    ctrl->setParent(w);

    // Initialize the Rule edit dialog
    {
        const auto ruleRow = ctrl->ruleListModel()->ruleRowById(ruleId);
        w->initialize(ruleRow);
    }

    DialogUtil::showDialog(w);

    return w;
}
