#include "tableitemmodel.h"

TableItemModel::TableItemModel(QObject *parent) : QAbstractItemModel(parent) { }

QModelIndex TableItemModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex TableItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);

    return {};
}

QModelIndex TableItemModel::sibling(int row, int column, const QModelIndex &index) const
{
    Q_UNUSED(index);

    return this->index(row, column);
}

bool TableItemModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() && rowCount() > 0;
}

Qt::ItemFlags TableItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren
            | flagIsUserCheckable(index);
}

Qt::ItemFlags TableItemModel::flagIsUserCheckable(const QModelIndex & /*index*/) const
{
    return Qt::NoItemFlags;
}

void TableItemModel::reset()
{
    beginResetModel();
    invalidateRowCache();
    endResetModel();
}

void TableItemModel::refresh()
{
    invalidateRowCache();

    const auto firstCell = index(0, 0);
    const auto lastCell = index(rowCount() - 1, columnCount(firstCell) - 1);

    emit dataChanged(firstCell, lastCell);
}

void TableItemModel::invalidateRowCache()
{
    tableRow().invalidate();
}

void TableItemModel::updateRowCache(int row) const
{
    if (tableRow().isValid(row))
        return;

    if (row >= 0 && !updateTableRow(row)) {
        row = -1;
    }

    tableRow().row = row;
}
