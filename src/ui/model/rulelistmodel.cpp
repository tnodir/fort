#include "rulelistmodel.h"

#include <QFont>
#include <QIcon>
#include <QLoggingCategory>

#include <sqlite/dbutil.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <util/guiutil.h>
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
    auto confRuleManager = IoC<ConfRuleManager>();

    connect(confRuleManager, &ConfRuleManager::ruleAdded, this, &TableItemModel::reset);
    connect(confRuleManager, &ConfRuleManager::ruleRemoved, this, &TableItemModel::reset);
    connect(confRuleManager, &ConfRuleManager::ruleUpdated, this, &TableItemModel::refresh);
}

QModelIndex RuleListModel::indexByRuleType(Rule::RuleType ruleType) const
{
    return createIndex(ruleType, 0, ruleType);
}

QModelIndex RuleListModel::index(int row, int column, const QModelIndex &parent) const
{
    int id = row;
    if (parent.isValid()) {
        id = parent.internalId();
        if (id < 0)
            return QModelIndex();

        id = -(id + 1);
    }

    return createIndex(row, column, id);
}

QModelIndex RuleListModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    const int id = child.internalId();
    if (id >= 0)
        return QModelIndex();

    const int row = -(id + 1);

    return createIndex(row, 0, row);
}

int RuleListModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return Rule::RuleTypeCount;

    const int id = parent.internalId();
    if (id < 0)
        return 0;

    setSqlRuleType(id);

    return FtsTableSqlModel::rowCount(parent);
}

int RuleListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
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

    const QModelIndex parent = index.parent();
    if (!parent.isValid()) {
        return rootData(index, role);
    }

    setSqlRuleType(parent.row());

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

QVariant RuleListModel::rootData(const QModelIndex &index, int role) const
{
    if (index.column() > 0)
        return QVariant();

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return ruleTypeNames().value(index.row());

    // Font
    case Qt::FontRole:
        return GuiUtil::fontBold();
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

bool RuleListModel::setData(const QModelIndex &index, const QVariant & /*value*/, int role)
{
    if (!index.isValid())
        return false;

    const QModelIndex parent = index.parent();
    setSqlRuleType(parent.row());

    switch (role) {
    case Qt::CheckStateRole:
        const auto ruleRow = ruleRowAt(index.row());
        return confRuleManager()->updateRuleEnabled(ruleRow.ruleId, !ruleRow.enabled);
    }

    return false;
}

Qt::ItemFlags RuleListModel::flagHasChildren(const QModelIndex &index) const
{
    const QModelIndex parent = index.parent();

    return parent.isValid() ? Qt::ItemNeverHasChildren : Qt::NoItemFlags;
}

Qt::ItemFlags RuleListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    const QModelIndex parent = index.parent();

    return parent.isValid() && index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

void RuleListModel::fillQueryVars(QVariantHash &vars) const
{
    FtsTableSqlModel::fillQueryVars(vars);

    vars.insert(":type", sqlRuleType());
}

const RuleRow &RuleListModel::ruleRowAt(int row) const
{
    updateRowCache(row);

    return m_ruleRow;
}

bool RuleListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    return updateRuleRow(sql(), vars, m_ruleRow);
}

bool RuleListModel::updateRuleRow(
        const QString &sql, const QVariantHash &vars, RuleRow &ruleRow) const
{
    SqliteStmt stmt;
    if (!DbUtil(sqliteDb()).sql(sql).vars(vars).prepareRow(stmt)) {
        ruleRow.invalidate();
        return false;
    }

    ruleRow.ruleId = stmt.columnInt(0);
    ruleRow.enabled = stmt.columnBool(1);
    ruleRow.blocked = stmt.columnBool(2);
    ruleRow.exclusive = stmt.columnBool(3);
    ruleRow.ruleName = stmt.columnText(4);
    ruleRow.notes = stmt.columnText(5);
    ruleRow.ruleText = stmt.columnText(6);
    ruleRow.ruleType = Rule::RuleType(stmt.columnInt(7));
    ruleRow.acceptZones = stmt.columnUInt(8);
    ruleRow.rejectZones = stmt.columnUInt(9);
    ruleRow.modTime = stmt.columnDateTime(10);

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
           "    rule_type,"
           "    accept_zones,"
           "    reject_zones,"
           "    mod_time"
           "  FROM rule t"
           "  WHERE rule_type = :type";
}

QString RuleListModel::sqlWhereFts() const
{
    return " AND t.rule_id IN ( SELECT rowid FROM rule_fts(:match) )";
}

QString RuleListModel::sqlOrderColumn() const
{
    return "rule_type, lower(name)";
}

void RuleListModel::setSqlRuleType(qint8 v) const
{
    if (m_sqlRuleType == v)
        return;

    m_sqlRuleType = v;

    invalidateRowCache();
}

QStringList RuleListModel::ruleTypeNames()
{
    return {
        RuleListModel::tr("Application Rules"),
        RuleListModel::tr("Global Rules, applied before App Rules"),
        RuleListModel::tr("Global Rules, applied after App Rules"),
        RuleListModel::tr("Preset Rules"),
    };
}
