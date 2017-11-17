#include "stringlistmodel.h"

StringListModel::StringListModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

int StringListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_list.size();
}

QVariant StringListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.isValid()) {
        return m_list.at(index.row());
    }
    return QVariant();
}

void StringListModel::setList(const QStringList &list)
{
    beginResetModel();
    m_list = list;
    endResetModel();
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
    m_list.removeAt(row);
    endRemoveRows();
}

void StringListModel::replace(const QString &text, int row)
{
    row = adjustRow(row);

    m_list.replace(row, text);

    const QModelIndex modelIndex = index(row);

    dataChanged(modelIndex, modelIndex);
}

int StringListModel::adjustRow(int row) const
{
    return (row < 0) ? (m_list.size() + 1 + row) : row;
}
