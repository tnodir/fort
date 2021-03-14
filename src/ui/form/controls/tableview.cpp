#include "tableview.h"

#include <QContextMenuEvent>
#include <QMenu>

TableView::TableView(QWidget *parent) : QTableView(parent) { }

int TableView::currentRow() const
{
    return currentIndex().row();
}

QVector<int> TableView::selectedRows() const
{
    QSet<int> rowsSet;
    const auto indexes = selectedIndexes();
    for (const auto index : indexes) {
        rowsSet.insert(index.row());
    }
    rowsSet.insert(currentRow());

    auto rows = rowsSet.values();
    std::sort(rows.begin(), rows.end());
    return rows.toVector();
}

QModelIndexList TableView::sortedSelectedIndexes() const
{
    auto indexes = selectedIndexes();
    std::sort(indexes.begin(), indexes.end());
    return indexes;
}

QString TableView::selectedText() const
{
    QString text;

    int prevRow = -1;
    int prevColumn = -1;

    const auto indexes = sortedSelectedIndexes();
    for (const auto index : indexes) {
        const int row = index.row();
        if (prevRow != row) {
            if (prevRow != -1) {
                text.append('\n');
            }
            prevRow = row;
            prevColumn = -1;
        }

        const int column = index.column();
        if (prevColumn != column) {
            if (prevColumn != -1) {
                text.append('\t');
            }
            prevColumn = column;
        }

        const QString s = model()->data(index).toString();
        text.append(s);
    }

    return text;
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
