#include "listview.h"

ListView::ListView(QWidget *parent) :
    QListView(parent)
{
}

void ListView::currentChanged(const QModelIndex &current,
                              const QModelIndex &previous)
{
    QListView::currentChanged(current, previous);

    emit currentIndexChanged(current);
}
