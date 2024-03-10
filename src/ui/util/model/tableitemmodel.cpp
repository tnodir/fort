#include "tableitemmodel.h"

#include <util/triggertimer.h>

TableItemModel::TableItemModel(QObject *parent) : QAbstractItemModel(parent) { }

QModelIndex TableItemModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex TableItemModel::parent(const QModelIndex & /*child*/) const
{
    return {};
}

bool TableItemModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}

Qt::ItemFlags TableItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | flagHasChildren(index)
            | flagIsUserCheckable(index);
}

void TableItemModel::resetLater()
{
    if (!m_resetTimer) {
        m_resetTimer = new TriggerTimer(this);

        connect(m_resetTimer, &QTimer::timeout, this, &TableItemModel::reset);
    }

    m_resetTimer->startTrigger();
}

Qt::ItemFlags TableItemModel::flagHasChildren(const QModelIndex & /*index*/) const
{
    return Qt::ItemNeverHasChildren;
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

void TableItemModel::invalidateRowCache() const
{
    tableRow().invalidate();
}

void TableItemModel::updateRowCache(int row) const
{
    if (tableRow().isValid(row))
        return;

    if (row >= 0) {
        QVariantHash vars;
        fillQueryVarsForRow(vars, row);

        if (!updateTableRow(vars, row)) {
            row = -1;
        }
    }

    tableRow().row = row;
}

void TableItemModel::fillQueryVars(QVariantHash & /*vars*/) const { }

void TableItemModel::fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const { }
