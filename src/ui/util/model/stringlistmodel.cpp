#include "stringlistmodel.h"

StringListModel::StringListModel(QObject *parent) : QAbstractListModel(parent) { }

int StringListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_list.size();
}

QVariant StringListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        return m_list.at(index.row());
    }

    return {};
}

Qt::ItemFlags StringListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}

void StringListModel::setList(const QStringList &list)
{
    beginResetModel();
    m_list = list;
    endResetModel();
}

void StringListModel::clear()
{
    setList({});
}

void StringListModel::insert(const QString &text, int row)
{
    row = adjustRow(row);

    beginInsertRows(QModelIndex(), row, row);
    m_list.insert(row, text);
    endInsertRows();
}

void StringListModel::remove(int row)
{
    row = adjustRow(row);

    beginRemoveRows(QModelIndex(), row, row);
    removeRow(row);
    endRemoveRows();
}

void StringListModel::replace(const QString &text, int row)
{
    row = adjustRow(row);

    m_list.replace(row, text);

    const QModelIndex modelIndex = index(row);

    emit dataChanged(modelIndex, modelIndex);
}

bool StringListModel::canMove(int fromRow, int toRow) const
{
    Q_ASSERT(fromRow != toRow);

    if (fromRow < 0 || toRow < 0)
        return false;

    const int listSize = list().size();
    if (fromRow >= listSize || toRow >= listSize)
        return false;

    return true;
}

void StringListModel::move(int fromRow, int toRow)
{
    beginMoveRows(
            QModelIndex(), fromRow, fromRow, QModelIndex(), toRow + (toRow > fromRow ? 1 : 0));
    m_list.move(fromRow, toRow);
    endMoveRows();
}

void StringListModel::reset()
{
    if (isChanging())
        return;

    beginResetModel();
    endResetModel();
}

void StringListModel::refresh()
{
    const auto firstCell = index(0, 0);
    const auto lastCell = index(rowCount() - 1, 0);

    emit dataChanged(firstCell, lastCell);
}

void StringListModel::removeRow(int row)
{
    m_list.removeAt(row);
}

int StringListModel::adjustRow(int row) const
{
    return (row < 0) ? (m_list.size() + 1 + row) : row;
}

void StringListModel::doBeginRemoveRows(int first, int last, const QModelIndex &parent)
{
    QAbstractItemModel::beginRemoveRows(parent, first, last);
    setIsChanging(true);
}

void StringListModel::doEndRemoveRows()
{
    setIsChanging(false);
    QAbstractItemModel::endRemoveRows();
}
