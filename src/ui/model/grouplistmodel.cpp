#include "grouplistmodel.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confgroupmanager.h>
#include <conf/confmanager.h>
#include <util/conf/confutil.h>
#include <util/ioc/ioccontainer.h>

namespace {
const QLoggingCategory LC("model.groupList");
}

GroupListModel::GroupListModel(QObject *parent) : TableSqlModel(parent) { }

ConfManager *GroupListModel::confManager() const
{
    return IoC<ConfManager>();
}

ConfGroupManager *GroupListModel::confGroupManager() const
{
    return IoC<ConfGroupManager>();
}

SqliteDb *GroupListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void GroupListModel::setUp()
{
    auto confGroupManager = IoCDependency<ConfGroupManager>();

    connect(confGroupManager, &ConfGroupManager::groupAdded, this, &TableItemModel::reset);
    connect(confGroupManager, &ConfGroupManager::groupRemoved, this, &TableItemModel::reset);
    connect(confGroupManager, &ConfGroupManager::groupUpdated, this, &TableItemModel::refresh);
}

int GroupListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant GroupListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        return headerDataDisplay(section);
    }
    return {};
}

QVariant GroupListModel::data(const QModelIndex &index, int role) const
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

QVariant GroupListModel::headerDataDisplay(int section) const
{
    switch (section) {
    case 0:
        return tr("Group");
    case 1:
        return tr("Change Time");
    }
    return {};
}

QVariant GroupListModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &groupRow = groupRowAt(row);

    switch (column) {
    case 0:
        return QString("%1) %2").arg(QString::number(groupRow.groupId), groupRow.groupName);
    case 1:
        return groupRow.modTime;
    }

    return {};
}

QVariant GroupListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto &groupRow = groupRowAt(index.row());
        return groupRow.enabled ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

bool GroupListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto &groupRow = groupRowAt(index.row());
        return confGroupManager()->updateGroupEnabled(groupRow.groupId, !groupRow.enabled);
    }

    return false;
}

Qt::ItemFlags GroupListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

const GroupRow &GroupListModel::groupRowAt(int row) const
{
    updateRowCache(row);

    return m_groupRow;
}

bool GroupListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    return updateGroupRow(sql(), vars, m_groupRow);
}

bool GroupListModel::updateGroupRow(
        const QString &sql, const QVariantHash &vars, GroupRow &groupRow) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepareRow(stmt))
        return false;

    groupRow.groupId = stmt.columnInt(0);
    groupRow.enabled = stmt.columnBool(1);
    groupRow.exclusive = stmt.columnBool(2);
    groupRow.periodEnabled = stmt.columnBool(3);
    groupRow.groupName = stmt.columnText(4);
    groupRow.notes = stmt.columnText(5);
    groupRow.periodFrom = stmt.columnText(6);
    groupRow.periodTo = stmt.columnText(7);
    groupRow.ruleId = stmt.columnInt(8);
    groupRow.modTime = stmt.columnDateTime(9);

    return true;
}

QString GroupListModel::sqlBase() const
{
    return "SELECT"
           "    group_id,"
           "    enabled,"
           "    exclusive,"
           "    period_enabled,"
           "    name,"
           "    notes,"
           "    period_from,"
           "    period_to,"
           "    rule_id,"
           "    mod_time"
           "  FROM app_group2";
}
