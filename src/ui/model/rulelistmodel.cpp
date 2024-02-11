#include "rulelistmodel.h"

#include <QIcon>
#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

namespace {

const QLoggingCategory LC("model.ruleList");

QVariant dataDisplayName(const RuleRow &ruleRow, int role)
{
    return ruleRow.ruleName
            + (role == Qt::ToolTipRole && !ruleRow.notes.isEmpty() ? "\n\n" + ruleRow.notes
                                                                   : QString());
}

QString ruleStateIconPath(const RuleRow &ruleRow)
{
    if (ruleRow.blocked)
        return ":/icons/deny.png";

    return ":/icons/accept.png";
}

}

RuleListModel::RuleListModel(QObject *parent) : FtsTableSqlModel(parent)
{
    initialize();
}

ConfManager *RuleListModel::confManager() const
{
    return IoC<ConfManager>();
}

ConfRuleManager *RuleListModel::confRuleManager() const
{
    return IoC<ConfRuleManager>();
}

SqliteDb *RuleListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void RuleListModel::initialize()
{
    setSortColumn(1);
    setSortOrder(Qt::AscendingOrder);

    auto confRuleManager = IoC<ConfRuleManager>();

    connect(confRuleManager, &ConfRuleManager::ruleAdded, this, &TableItemModel::reset);
    connect(confRuleManager, &ConfRuleManager::ruleRemoved, this, &TableItemModel::reset);
    connect(confRuleManager, &ConfRuleManager::ruleUpdated, this, &TableItemModel::refresh);
}

int RuleListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant RuleListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        return headerDataDisplay(section);
    }
    return QVariant();
}

QVariant RuleListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

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

    return QVariant();
}

QVariant RuleListModel::headerDataDisplay(int section) const
{
    switch (section) {
    case 0:
        return tr("Rule");
    case 1:
        return tr("Change Time");
    }
    return QVariant();
}

QVariant RuleListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();
    const int column = index.column();

    const auto ruleRow = ruleRowAt(row);

    switch (column) {
    case 0:
        return dataDisplayName(ruleRow, role);
    case 1:
        return ruleRow.modTime;
    }

    return QVariant();
}

QVariant RuleListModel::dataDecoration(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    const auto ruleRow = ruleRowAt(row);
    if (ruleRow.isNull())
        return QVariant();

    switch (column) {
    case 0:
        return IconCache::icon(ruleStateIconPath(ruleRow));
    }

    return QVariant();
}

QVariant RuleListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto ruleRow = ruleRowAt(index.row());
        return ruleRow.enabled ? Qt::Checked : Qt::Unchecked;
    }

    return QVariant();
}

bool RuleListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto ruleRow = ruleRowAt(index.row());
        return confRuleManager()->updateRuleEnabled(ruleRow.ruleId, !ruleRow.enabled);
    }

    return false;
}

Qt::ItemFlags RuleListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

const RuleRow &RuleListModel::ruleRowAt(int row) const
{
    updateRowCache(row);

    return m_ruleRow;
}

bool RuleListModel::updateTableRow(int row) const
{
    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql(), { row }) && stmt.step() == SqliteStmt::StepRow)) {
        m_ruleRow.invalidate();
        return false;
    }

    m_ruleRow.ruleId = stmt.columnInt(0);
    m_ruleRow.enabled = stmt.columnBool(1);
    m_ruleRow.blocked = stmt.columnBool(2);
    m_ruleRow.exclusive = stmt.columnBool(3);
    m_ruleRow.ruleName = stmt.columnText(4);
    m_ruleRow.notes = stmt.columnText(5);
    m_ruleRow.ruleText = stmt.columnText(6);
    m_ruleRow.acceptZones = stmt.columnUInt(7);
    m_ruleRow.rejectZones = stmt.columnUInt(8);
    m_ruleRow.modTime = stmt.columnDateTime(9);

    return true;
}

QString RuleListModel::sqlBase() const
{
    return "SELECT"
           "    rule_id,"
           "    enabled,"
           "    blocked,"
           "    exclusive,"
           "    name,"
           "    notes,"
           "    rule_text,"
           "    accept_zones,"
           "    reject_zones,"
           "    mod_time"
           "  FROM rule t";
}

QString RuleListModel::sqlWhereFts() const
{
    return " WHERE t.rule_id IN ( SELECT rowid FROM rule_fts(:match) )";
}

QString RuleListModel::sqlOrderColumn() const
{
    return "t.name";
}
