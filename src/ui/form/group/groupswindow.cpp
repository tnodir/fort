#include "groupswindow.h"

#include <QHeaderView>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/dialog/dialogutil.h>
#include <manager/windowmanager.h>
#include <model/grouplistmodel.h>
#include <user/iniuser.h>
#include <util/conf/confutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "groupeditdialog.h"
#include "groupscontroller.h"

namespace {

constexpr int GROUPS_HEADER_VERSION = 1;

}

GroupsWindow::GroupsWindow(QWidget *parent) : FormWindow(parent), m_ctrl(new GroupsController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::groupWindowGroup());
}

WindowManager *GroupsWindow::windowManager() const
{
    return ctrl()->windowManager();
}

GroupListModel *GroupsWindow::groupListModel() const
{
    return ctrl()->groupListModel();
}

void GroupsWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setGroupWindowGeometry(stateWatcher()->geometry());
    iniUser()->setGroupWindowMaximized(stateWatcher()->maximized());

    auto header = m_groupListView->horizontalHeader();
    iniUser()->setGroupsHeader(header->saveState());
    iniUser()->setGroupsHeaderVersion(GROUPS_HEADER_VERSION);

    confManager()->saveIniUser();
}

void GroupsWindow::restoreWindowState()
{
    stateWatcher()->restore(this, QSize(500, 600), iniUser()->groupWindowGeometry(),
            iniUser()->groupWindowMaximized());

    if (iniUser()->groupsHeaderVersion() == GROUPS_HEADER_VERSION) {
        auto header = m_groupListView->horizontalHeader();
        header->restoreState(iniUser()->groupsHeader());
    }
}

void GroupsWindow::setupController()
{
    connect(ctrl(), &GroupsController::retranslateUi, this, &GroupsWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void GroupsWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddGroup->setText(tr("Add"));
    m_actEditGroup->setText(tr("Edit"));
    m_actRemoveGroup->setText(tr("Remove"));

    groupListModel()->refresh();

    this->setWindowTitle(tr("Groups"));
}

void GroupsWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableGroups();
    setupTableGroupsHeader();

    // Actions on groups table's current changed
    setupTableGroupsChanged();

    // Actions on group list model's changed
    setupGroupListModelChanged();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_groupListView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 600);
}

QLayout *GroupsWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddGroup = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddGroup->setShortcut(Qt::Key_Plus);

    m_actEditGroup = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditGroup->setShortcut(Qt::Key_Enter);

    m_actRemoveGroup = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveGroup->setShortcut(Qt::Key_Delete);

    connect(m_actAddGroup, &QAction::triggered, this, &GroupsWindow::addNewGroup);
    connect(m_actEditGroup, &QAction::triggered, this, &GroupsWindow::editSelectedGroup);
    connect(m_actRemoveGroup, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { deleteSelectedGroup(); }, tr("Are you sure to remove selected Рїroup?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btEdit, /*stretch*/ nullptr, m_btOptions, m_btMenu });

    return layout;
}

void GroupsWindow::setupTableGroups()
{
    m_groupListView = new TableView();
    m_groupListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_groupListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_groupListView->setModel(groupListModel());

    m_groupListView->setMenu(m_btEdit->menu());

    connect(m_groupListView, &TableView::activated, m_actEditGroup, &QAction::trigger);
}

void GroupsWindow::setupTableGroupsHeader()
{
    auto header = m_groupListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(0, 360);
    header->resizeSection(1, 130);
}

void GroupsWindow::setupTableGroupsChanged()
{
    const auto refreshTableGroupsChanged = [&] {
        const int groupIndex = groupListCurrentIndex();
        const bool groupSelected = (groupIndex >= 0);
        m_actEditGroup->setEnabled(groupSelected);
        m_actRemoveGroup->setEnabled(groupSelected);
    };

    refreshTableGroupsChanged();

    connect(m_GroupListView, &TableView::currentIndexChanged, this, refreshTableGroupsChanged);
}

void GroupsWindow::setupGroupListModelChanged()
{
    const auto refreshAddGroup = [&] {
        m_actAddGroup->setEnabled(groupListModel()->rowCount() < ConfUtil::groupMaxCount());
    };

    refreshAddGroup();

    connect(groupListModel(), &GroupListModel::modelReset, this, refreshAddGroup);
    connect(groupListModel(), &GroupListModel::rowsRemoved, this, refreshAddGroup);
}

void GroupsWindow::addNewGroup()
{
    openGroupEditForm({});
}

void GroupsWindow::editSelectedGroup()
{
    const int groupIndex = groupListCurrentIndex();
    if (groupIndex < 0)
        return;

    const auto &groupRow = groupListModel()->groupRowAt(groupIndex);

    openGroupEditForm(groupRow);
}

void GroupsWindow::openGroupEditForm(const GroupRow &groupRow)
{
    if (!m_formGroupEdit) {
        m_formGroupEdit = new GroupEditDialog(ctrl(), this);
    }

    m_formGroupEdit->initialize(groupRow);

    WidgetWindow::showWidget(m_formGroupEdit);
}

void GroupsWindow::deleteGroup(int row)
{
    const auto &groupRow = groupListModel()->groupRowAt(row);
    if (groupRow.isNull())
        return;

    ctrl()->deleteGroup(groupRow.groupId);
}

void GroupsWindow::deleteSelectedGroup()
{
    deleteGroup(groupListCurrentIndex());
}

int GroupsWindow::groupListCurrentIndex() const
{
    return m_groupListView->currentRow();
}
