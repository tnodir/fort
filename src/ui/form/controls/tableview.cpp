#include "tableview.h"

TableView::TableView(QWidget *parent) :
    QTableView(parent)
{
}

void TableView::currentChanged(const QModelIndex &current,
                               const QModelIndex &previous)
{
    QTableView::currentChanged(current, previous);

    emit currentIndexChanged(current);
}
