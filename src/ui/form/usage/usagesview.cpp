#include "usagesview.h"

#include <form/group/groupswindow.h>
#include <form/opt/pages/optmainpage.h>
#include <form/rule/ruleswindow.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/usagesmodel.h>

using namespace Fort;

UsagesView::UsagesView(QWidget *parent) : TreeView(parent), m_usagesModel(new UsagesModel(this))
{
    setHeaderHidden(true);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    setupItemDelegate();

    setModel(m_usagesModel);

    connect(this, &TreeView::activated, this, &UsagesView::openUsage);
}

void UsagesView::setUsages(const ConfUsages &usages)
{
    m_usagesModel->setUsages(usages);
    m_usagesLoaded = true;

    expandAll();
}

void UsagesView::clearUsages()
{
    m_usagesModel->setUsages({});
    m_usagesLoaded = false;
}

void UsagesView::retranslateUi()
{
    m_usagesModel->refresh();
}

void UsagesView::showEvent(QShowEvent *event)
{
    TreeView::showEvent(event);

    if (!m_usagesLoaded) {
        emit usagesRequested();
    }
}

void UsagesView::openUsage(const QModelIndex &index)
{
    if (UsagesModel::isIndexRoot(index))
        return;

    const auto &usage = m_usagesModel->usageAt(index);

    switch (m_usagesModel->indexCategory(index)) {
    case ConfUsages::CategoryAddressLists:
        windowManager()->showOptionsWindowTab(OptMainPage::TabAddresses);
        break;
    case ConfUsages::CategoryApps:
        windowManager()->openProgramEditForm(QString(), usage.id);
        break;
    case ConfUsages::CategoryGroups:
        GroupsWindow::showGroupEditDialog(usage.id, this);
        break;
    default:
        RulesWindow::showRuleEditDialog(usage.id, this);
    }
}
