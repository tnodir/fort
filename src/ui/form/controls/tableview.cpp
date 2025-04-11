#include "tableview.h"

#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>

#include <util/guiutil.h>

TableView::TableView(QWidget *parent) : QTableView(parent)
{
    setWordWrap(false);
    setTextElideMode(Qt::ElideMiddle);
    setAlternatingRowColors(true);
    setTabKeyNavigation(false);
}

void TableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);

    connect(model, &QAbstractItemModel::modelReset, this,
            [&] { emit currentIndexChanged(currentIndex()); });
}

int TableView::currentRow() const
{
    return currentIndex().row();
}

QVector<int> TableView::selectedRows() const
{
    QSet<int> rowsSet;
    const auto indexes = selectedIndexes();
    for (const auto &index : indexes) {
        rowsSet.insert(index.row());
    }

    const int row = currentRow();
    if (row >= 0) {
        rowsSet.insert(row);
    }

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
    for (const auto &index : indexes) {
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

        text.append(cellText(index));
    }

    return text;
}

QString TableView::cellText(const QModelIndex &index) const
{
    const QString displayText = model()->data(index).toString();
    if (!displayText.isEmpty())
        return displayText;

    const QString tooltipText = model()->data(index, Qt::ToolTipRole).toString();
    return tooltipText;
}

void TableView::selectCell(int row, int column)
{
    const auto index = model()->index(row, column);
    this->setCurrentIndex(index);
    this->scrollTo(index);
}

void TableView::copySelectedText()
{
    GuiUtil::setClipboardData(selectedText());
}

void TableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTableView::selectionChanged(selected, deselected);

    if (selected.isEmpty())
        return;

    if (!currentIndex().isValid()) {
        setCurrentIndex(selected.indexes().first());
    }
}

void TableView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTableView::currentChanged(current, previous);

    emit currentIndexChanged(current);
}

void TableView::contextMenuEvent(QContextMenuEvent *event)
{
    if (menu()) {
        menu()->popup(event->globalPos());
    }
}

void TableView::keyPressEvent(QKeyEvent *event)
{
    if (event == QKeySequence::Copy) {
        copySelectedText();
        event->accept();
        return;
    }

    QAbstractItemView::keyPressEvent(event);
}
