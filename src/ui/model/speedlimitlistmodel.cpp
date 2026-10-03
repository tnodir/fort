#include "speedlimitlistmodel.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/confspeedlimitmanager.h>
#include <fortglobal.h>
#include <model/connlistmodel.h>
#include <util/iconcache.h>

using namespace Fort;

namespace {

const QLoggingCategory LC("model.speedLimitList");

using dataDisplay_func = QVariant (*)(const SpeedLimitRow &speedLimitRow, int role);

QVariant dataDisplayName(const SpeedLimitRow &speedLimitRow, int /*role*/)
{
    return QString("%1) %2").arg(QString::number(speedLimitRow.limitId), speedLimitRow.menuLabel());
}

QVariant dataDisplayDirection(const SpeedLimitRow &speedLimitRow, int role)
{
    if (role != Qt::ToolTipRole)
        return {};

    return ConnListModel::directionText(speedLimitRow.inbound);
}

QVariant dataDisplayModTime(const SpeedLimitRow &speedLimitRow, int /*role*/)
{
    return speedLimitRow.modTime;
}

static const dataDisplay_func dataDisplay_funcList[] = {
    &dataDisplayName,
    &dataDisplayDirection,
    &dataDisplayModTime,
};

static_assert(std::size(dataDisplay_funcList) == int(SpeedLimitListColumn::Count));

}

SpeedLimitListModel::SpeedLimitListModel(QObject *parent) : TableSqlModel(parent) { }

SqliteDb *SpeedLimitListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void SpeedLimitListModel::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();
    auto confSpeedLimitManager = Fort::dependency<ConfSpeedLimitManager>();

    connect(confManager, &ConfManager::imported, this, &TableItemModel::reset);

    connect(confSpeedLimitManager, &ConfSpeedLimitManager::speedLimitAdded, this,
            &TableItemModel::reset);
    connect(confSpeedLimitManager, &ConfSpeedLimitManager::speedLimitRemoved, this,
            &TableItemModel::reset);
    connect(confSpeedLimitManager, &ConfSpeedLimitManager::speedLimitUpdated, this,
            &TableItemModel::refresh);
}

int SpeedLimitListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(SpeedLimitListColumn::Count);
}

QVariant SpeedLimitListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return headerDataDisplay(section, role);

    // Icon
    case Qt::DecorationRole:
        return headerDataDecoration(section);
    }

    return {};
}

QVariant SpeedLimitListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index, role);

    // Icon
    case Qt::DecorationRole:
        return dataDecoration(index);

    // Enabled
    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return {};
}

QVariant SpeedLimitListModel::headerDataDisplay(int section, int role) const
{
    const auto column = SpeedLimitListColumn(section);

    if (role == Qt::DisplayRole && column == SpeedLimitListColumn::Direction)
        return {};

    return columnName(column);
}

QVariant SpeedLimitListModel::headerDataDecoration(int section) const
{
    if (SpeedLimitListColumn(section) == SpeedLimitListColumn::Direction)
        return IconCache::icon(":/icons/green_down.png");

    return {};
}

QVariant SpeedLimitListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int column = index.column();

    const auto &speedLimitRow = speedLimitRowAt(index.row());
    if (speedLimitRow.isNull())
        return {};

    const dataDisplay_func func = dataDisplay_funcList[column];

    return func(speedLimitRow, role);
}

QVariant SpeedLimitListModel::dataDecoration(const QModelIndex &index) const
{
    if (SpeedLimitListColumn(index.column()) != SpeedLimitListColumn::Direction)
        return {};

    const auto &speedLimitRow = speedLimitRowAt(index.row());
    if (speedLimitRow.isNull())
        return {};

    return IconCache::icon(ConnListModel::directionIconPath(speedLimitRow.inbound));
}

QVariant SpeedLimitListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto &speedLimitRow = speedLimitRowAt(index.row());
        return speedLimitRow.enabled ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

bool SpeedLimitListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto &speedLimitRow = speedLimitRowAt(index.row());
        return confSpeedLimitManager()->updateSpeedLimitEnabled(
                speedLimitRow.limitId, !speedLimitRow.enabled);
    }

    return false;
}

Qt::ItemFlags SpeedLimitListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

const SpeedLimitRow &SpeedLimitListModel::speedLimitRowAt(int row) const
{
    updateRowCache(row);

    return m_speedLimitRow;
}

QString SpeedLimitListModel::columnName(const SpeedLimitListColumn column)
{
    const QStringList columnNames = {
        tr("Speed Limit"),
        tr("Direction"),
        tr("Change Time"),
    };

    return columnNames.value(int(column));
}

bool SpeedLimitListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    return updateSpeedLimitRow(sql(), vars, m_speedLimitRow);
}

bool SpeedLimitListModel::updateSpeedLimitRow(
        const QString &sql, const QVariantHash &vars, SpeedLimitRow &speedLimitRow) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepareRow(stmt))
        return false;

    speedLimitRow.limitId = stmt.columnInt(0);
    speedLimitRow.enabled = stmt.columnBool(1);
    speedLimitRow.inbound = stmt.columnBool(2);
    speedLimitRow.packetLoss = stmt.columnInt(3);
    speedLimitRow.latency = stmt.columnUInt(4);
    speedLimitRow.kbps = stmt.columnUInt(5);
    speedLimitRow.bufferSize = stmt.columnUInt(6);
    speedLimitRow.name = stmt.columnText(7);
    speedLimitRow.notes = stmt.columnText(8);
    speedLimitRow.modTime = stmt.columnDateTime(9);

    return true;
}

QString SpeedLimitListModel::sqlBase() const
{
    return "SELECT"
           "    limit_id,"
           "    enabled,"
           "    inbound,"
           "    packet_loss,"
           "    latency,"
           "    kbps,"
           "    bufsize,"
           "    name,"
           "    notes,"
           "    mod_time"
           "  FROM speed_limit";
}
