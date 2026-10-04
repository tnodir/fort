#include "timeperiodlistmodel.h"

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/conftimeperiodmanager.h>
#include <fortglobal.h>

using namespace Fort;

TimePeriodListModel::TimePeriodListModel(QObject *parent) : TableSqlModel(parent) { }

SqliteDb *TimePeriodListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void TimePeriodListModel::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();
    auto confTimePeriodManager = Fort::dependency<ConfTimePeriodManager>();

    connect(confManager, &ConfManager::imported, this, &TableItemModel::reset);

    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodAdded, this,
            &TableItemModel::reset);
    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodRemoved, this,
            &TableItemModel::reset);
    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodUpdated, this,
            &TableItemModel::refresh);
}

int TimePeriodListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(TimePeriodListColumn::Count);
}

QVariant TimePeriodListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return columnName(TimePeriodListColumn(section));
    }

    return {};
}

QVariant TimePeriodListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);

    // Enabled
    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return {};
}

QVariant TimePeriodListModel::dataDisplay(const QModelIndex &index) const
{
    const auto &timePeriodRow = timePeriodRowAt(index.row());
    if (timePeriodRow.isNull())
        return {};

    switch (TimePeriodListColumn(index.column())) {
    case TimePeriodListColumn::Name:
        return QString("%1) %2").arg(QString::number(timePeriodRow.periodId), timePeriodRow.name);
    case TimePeriodListColumn::Intervals:
        return timePeriodRow.intervalsText();
    case TimePeriodListColumn::ModTime:
        return timePeriodRow.modTime;
    default:
        return {};
    }
}

QVariant TimePeriodListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto &timePeriodRow = timePeriodRowAt(index.row());
        return timePeriodRow.enabled ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

bool TimePeriodListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto &timePeriodRow = timePeriodRowAt(index.row());
        return confTimePeriodManager()->updateTimePeriodEnabled(
                timePeriodRow.periodId, !timePeriodRow.enabled);
    }

    return false;
}

Qt::ItemFlags TimePeriodListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

const TimePeriodRow &TimePeriodListModel::timePeriodRowAt(int row) const
{
    updateRowCache(row);

    return m_timePeriodRow;
}

QString TimePeriodListModel::columnName(const TimePeriodListColumn column)
{
    const QStringList columnNames = {
        tr("Time Period"),
        tr("Intervals"),
        tr("Change Time"),
    };

    return columnNames.value(int(column));
}

bool TimePeriodListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    return updateTimePeriodRow(sql(), vars, m_timePeriodRow);
}

bool TimePeriodListModel::updateTimePeriodRow(
        const QString &sql, const QVariantHash &vars, TimePeriodRow &timePeriodRow) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepareRow(stmt))
        return false;

    timePeriodRow.periodId = stmt.columnInt(0);
    timePeriodRow.enabled = stmt.columnBool(1);
    timePeriodRow.name = stmt.columnText(2);
    timePeriodRow.notes = stmt.columnText(3);
    timePeriodRow.modTime = stmt.columnDateTime(4);

    timePeriodRow.intervals.clear();

    return confTimePeriodManager()->loadTimePeriodIntervals(
            timePeriodRow.intervals, timePeriodRow.periodId);
}

QString TimePeriodListModel::sqlBase() const
{
    return "SELECT"
           "    period_id,"
           "    enabled,"
           "    name,"
           "    notes,"
           "    mod_time"
           "  FROM time_period";
}
