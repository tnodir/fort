#include "tableview.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>
#include <QStyle>

#include <algorithm>

#include <manager/windowmanager.h>
#include <util/guiutil.h>

TableView::TableView(QWidget *parent) : QTableView(parent)
{
    setWordWrap(false);
    setTextElideMode(Qt::ElideMiddle);
    setAlternatingRowColors(true);
    setTabKeyNavigation(false);
    setIconSize(WindowManager::listIconSize());

    updateRowsHeight();

    connect(this, &QAbstractItemView::iconSizeChanged, this, &TableView::updateRowsHeight);
}

void TableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);

    connect(model, &QAbstractItemModel::modelReset, this, &TableView::onModelReset);
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

        const QString s = cellText(index).trimmed().replace('\n', '|');

        text.append(s);
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

int TableView::scrollOffset(int row) const
{
    return verticalScrollBar()->value() - rowScrollPos(row);
}

void TableView::setScrollOffset(int row, int offset)
{
    // Update the scroll bar's range
    doItemsLayout();

    verticalScrollBar()->setValue(rowScrollPos(row) + offset);
}

void TableView::setRowsHeight(int v)
{
    m_rowsHeight = v;

    updateRowsHeight();
}

void TableView::selectCell(int row, int column)
{
    const auto index = model()->index(row, column);
    this->setCurrentIndex(index);
    this->scrollTo(index);
}

void TableView::keepSelectionByIds()
{
    m_keptModel = qobject_cast<TableSqlModel *>(model());
    Q_ASSERT(m_keptModel);

    connect(m_keptModel, &QAbstractItemModel::dataChanged, this, [this] { restoreSelection(); });
}

void TableView::rememberSelection()
{
    if (!m_keptModel || m_restoring)
        return;

    const auto rows = selectedRows();
    m_keptIds = m_keptModel->idsByRows(rows);

    // The current row is among the selected ones; 0 is no id (SQLite's ids start from 1)
    const int index = rows.indexOf(currentRow());
    m_keptCurrentId = (index >= 0 && index < m_keptIds.size()) ? m_keptIds.at(index) : 0;
}

void TableView::restoreSelection()
{
    // Taken first: the restored selection replaces the remembered one
    const auto idList = m_keptIds;
    const qint64 currentId = m_keptCurrentId;

    // Nothing was selected, or the same rows are still selected
    if (idList.isEmpty() || m_keptModel->idsByRows(selectedRows()) == idList)
        return;

    const auto rows = m_keptModel->rowsByIds(idList);

    // The rows are gone: after a refresh the selection would stay on other rows
    // (of the previous order)
    if (rows.isEmpty()) {
        clearSelection();
        m_keptIds.clear();
        return;
    }

    selectRowsByIds(rows, currentId);
}

void TableView::selectRowsByIds(const QVector<TableSqlModel::IdRow> &idRows, qint64 currentId)
{
    QVector<qint64> idList;
    QItemSelection selection;
    auto currentIt = idRows.cbegin(); // the first row, when the current id is gone

    for (auto it = idRows.cbegin(); it != idRows.cend(); ++it) {
        const auto index = model()->index(it->row, 0);
        selection.select(index, index);
        idList.append(it->id);

        if (it->id == currentId) {
            currentIt = it;
        }
    }

    // The restored ids are known: don't read them again on the selection's signals
    m_restoring = true;

    // Before the selection: else the view makes the first selected cell the current one,
    // which selects only it
    selectionModel()->setCurrentIndex(
            model()->index(currentIt->row, 0), QItemSelectionModel::NoUpdate);

    selectionModel()->select(
            selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    m_restoring = false;

    m_keptIds = idList;
    m_keptCurrentId = currentIt->id;
}

void TableView::onModelReset()
{
    selectResetRow();

    if (m_keptModel) {
        restoreSelection();
    }

    emit currentIndexChanged(currentIndex());
}

void TableView::selectResetRow()
{
    const int row = qMin(m_resetRow, model()->rowCount() - 1);

    m_resetRow = -1;

    if (row >= 0) {
        selectCell(row);
    }
}

void TableView::copySelectedText()
{
    GuiUtil::setClipboardData(selectedText());
}

void TableView::changeEvent(QEvent *event)
{
    QTableView::changeEvent(event);

    switch (event->type()) {
    case QEvent::FontChange:
    case QEvent::StyleChange:
        updateRowsHeight();
        break;
    default:
        break;
    }
}

void TableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTableView::selectionChanged(selected, deselected);

    if (selected.isEmpty()) {
        // The current cell doesn't outlive the selection (e.g. a click below the rows)
        if (selectionModel()->selection().isEmpty()) {
            selectionModel()->clearCurrentIndex();
        }
    } else if (!currentIndex().isValid()) {
        setCurrentIndex(selected.indexes().first());
    }

    rememberSelection();
}

void TableView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTableView::currentChanged(current, previous);

    rememberSelection();

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

int TableView::rowScrollPos(int row) const
{
    if (verticalScrollMode() == QAbstractItemView::ScrollPerItem)
        return row;

    return verticalHeader()->sectionPosition(row);
}

void TableView::updateRowsHeight()
{
    const int rowsHeight = (m_rowsHeight > 0) ? m_rowsHeight : defaultRowsHeight();

    verticalHeader()->setDefaultSectionSize(rowsHeight);
}

int TableView::defaultRowsHeight() const
{
    // As QHeaderView's default section size, by the current font and icon size
    const auto header = verticalHeader();
    const auto style = header->style();

    const int margin = 2 * style->pixelMetric(QStyle::PM_HeaderMargin, nullptr, header);
    const int contentHeight = qMax(fontMetrics().height(), iconSize().height()) + margin;
    const int styleHeight =
            style->pixelMetric(QStyle::PM_HeaderDefaultSectionSizeVertical, nullptr, header);

    return qMax(contentHeight, styleHeight);
}
