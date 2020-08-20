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

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}

void TableItemModel::refresh()
{
    const auto firstCell = index(0, 0);
    const auto lastCell = index(rowCount() - 1, columnCount(firstCell) - 1);

    emit dataChanged(firstCell, lastCell);
}
