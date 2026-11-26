#include "graphlistmodel.h"

#include <conf/confmanager.h>
#include <form/graph/graph_types.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/inigraph.h>
#include <user/iniuser.h>

using namespace Fort;

GraphListModel::GraphListModel(QObject *parent) : TableItemModel(parent)
{
    setupGraphIds();

    connect(windowManager(), &WindowManager::graphWindowVisibilityChanged, this,
            &GraphListModel::refresh);
    connect(confManager(), &ConfManager::iniUserChanged, this, &GraphListModel::reset);
}

int GraphListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_graphIds.size();
}

int GraphListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(GraphListColumn::Count);
}

QVariant GraphListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return headerDataDisplay(section);
        }
    }
    return {};
}

QVariant GraphListModel::headerDataDisplay(int section) const
{
    return columnName(GraphListColumn(section));
}

QVariant GraphListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);

    // Visible
    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return {};
}

QVariant GraphListModel::dataDisplay(const QModelIndex &index) const
{
    const auto &graphRow = graphRowAt(index.row());
    if (graphRow.isNull())
        return {};

    switch (GraphListColumn(index.column())) {
    case GraphListColumn::Name:
        return QString("%1) %2").arg(QString::number(graphRow.graphId + 1), graphRow.name);
    case GraphListColumn::GraphType:
        return GraphTypes::graphTypeNames().value(graphRow.graphType);
    default:
        return {};
    }
}

QVariant GraphListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto &graphRow = graphRowAt(index.row());
        return graphRow.visible ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

bool GraphListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto &graphRow = graphRowAt(index.row());
        if (graphRow.visible) {
            windowManager()->closeGraphWindow(graphRow.graphId);
        } else {
            windowManager()->showGraphWindow(graphRow.graphId);
        }
        return true;
    }

    return false;
}

Qt::ItemFlags GraphListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

const GraphRow &GraphListModel::graphRowAt(int row) const
{
    updateRowCache(row);

    return m_graphRow;
}

QString GraphListModel::columnName(GraphListColumn column)
{
    const QStringList columnNames = {
        tr("Graph"),
        tr("Graph type"),
    };

    return columnNames.value(int(column));
}

void GraphListModel::reset()
{
    setupGraphIds();

    TableItemModel::reset();
}

bool GraphListModel::updateTableRow(const QVariantHash & /*vars*/, int row) const
{
    if (row >= m_graphIds.size())
        return false;

    const int graphId = m_graphIds[row];
    const IniGraph ini(Fort::iniUser(), graphId);

    m_graphRow.visible = windowManager()->isGraphWindowShown(graphId);
    m_graphRow.graphId = graphId;
    m_graphRow.graphType = ini.graphType();
    m_graphRow.name = ini.name();

    return true;
}

void GraphListModel::setupGraphIds()
{
    m_graphIds = Fort::iniUser().graphWindowIds();
}
