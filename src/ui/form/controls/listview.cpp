#include "listview.h"

ListView::ListView(QWidget *parent) : QListView(parent) { }

void ListView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);

    connect(model, &QAbstractItemModel::modelReset, this,
            [&] { emit currentIndexChanged(currentIndex()); });
}

int ListView::currentRow() const
{
    return currentIndex().row();
}

void ListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QListView::currentChanged(current, previous);

    emit currentIndexChanged(current);
}
