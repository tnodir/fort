#include "treeview.h"

#include <QContextMenuEvent>
#include <QMenu>

#include <util/model/tableitemmodel.h>

#include "treeitemdelegate.h"

TreeView::TreeView(QWidget *parent) : QTreeView(parent)
{
    setTabKeyNavigation(false);
    setUniformRowHeights(true);
    setAnimated(true);
}

void TreeView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    connect(model, &QAbstractItemModel::modelReset, this,
            [&] { emit currentIndexChanged(currentIndex()); });
}

void TreeView::setupItemDelegate()
{
    auto tid = new TreeItemDelegate(this);

    setItemDelegateForColumn(0, tid);
}

void TreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    if (selected.isEmpty())
        return;

    if (!currentIndex().isValid()) {
        setCurrentIndex(selected.indexes().first());
    }
}

void TreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);

    emit currentIndexChanged(current);
}

void TreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_menu) {
        m_menu->popup(event->globalPos());
    }
}
