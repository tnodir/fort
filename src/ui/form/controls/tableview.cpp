#include "tableview.h"

TableView::TableView(QWidget *parent) :
    QTableView(parent)
{
}

QVector<int> TableView::selectedRows() const
{
    QSet<int> rowsSet;
    for (const auto index : selectedIndexes()) {
        rowsSet.insert(index.row());
    }

    auto rows = rowsSet.values();
    std::sort(rows.begin(), rows.end());
    return rows;
}

void TableView::currentChanged(const QModelIndex &current,
                               const QModelIndex &previous)
{
    QTableView::currentChanged(current, previous);

    emit currentIndexChanged(current);
}
