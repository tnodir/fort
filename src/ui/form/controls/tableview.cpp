#include "tableview.h"

#include <QContextMenuEvent>
#include <QMenu>

TableView::TableView(QWidget *parent) : QTableView(parent) { }

QVector<int> TableView::selectedRows() const
{
    QSet<int> rowsSet;
    const auto indexes = selectedIndexes();
    for (const auto index : indexes) {
        rowsSet.insert(index.row());
    }
    rowsSet.insert(currentIndex().row());

    auto rows = rowsSet.values();
    std::sort(rows.begin(), rows.end());
    return rows.toVector();
}

void TableView::selectCell(int row, int column)
{
    const auto index = model()->index(row, column);
    this->setCurrentIndex(index);
    this->scrollTo(index);
}

void TableView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTableView::currentChanged(current, previous);

    emit currentIndexChanged(current);
}

void TableView::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_menu != nullptr) {
        m_menu->popup(event->globalPos());
    }
}
