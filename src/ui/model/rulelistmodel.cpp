#include "rulelistmodel.h"

#include <QFont>
#include <QIcon>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <conf/conftimeperiodmanager.h>
#include <fortglobal.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/net/actionrange.h>

using namespace Fort;

namespace {

const QLoggingCategory LC("model.ruleList");

constexpr bool isIndexIdRoot(const quint32 id)
{
    return (id & RuleListModel::InternalIdRoot) != 0;
}

constexpr bool isIndexIdRule(const quint32 id)
{
    return (id & RuleListModel::InternalIdRule) != 0;
}

QVariant dataDisplayName(const RuleRow &ruleRow, int role)
{
    return ruleRow.ruleName
            + (role == Qt::ToolTipRole && !ruleRow.notes.isEmpty() ? "\n\n" + ruleRow.notes
                                                                   : QString());
}

quint8 rulePeriodId(const RuleRow &ruleRow)
{
    return ruleRow.periodEnabled ? ruleRow.periodId : 0;
}

QVariant dataDisplayPeriod(const RuleRow &ruleRow, int role)
{
    if (role != Qt::ToolTipRole)
        return {};

    const quint8 periodId = rulePeriodId(ruleRow);
    if (periodId == 0)
        return {};

    return confTimePeriodManager()->timePeriodNameById(periodId);
}

QIcon rulePeriodIcon(const RuleRow &ruleRow)
{
    return (rulePeriodId(ruleRow) != 0) ? IconCache::icon(":/icons/clock.png") : QIcon();
}

QVariant dataDisplayTerminate(const RuleRow &ruleRow, int role)
{
    if (role != Qt::ToolTipRole || !ruleRow.terminate)
        return {};

    return RuleListModel::tr("Terminating Rule:") + ' '
            + ActionRange::actionTypeText(ruleRow.terminateActionTypeId());
}

QIcon ruleTerminateIcon(const RuleRow &ruleRow)
{
    if (!ruleRow.terminate)
        return {};

    return IconCache::icon(ActionRange::actionTypeIconPath(ruleRow.terminateActionTypeId()));
}

quint8 ruleActionTypeId(const RuleRow &ruleRow)
{
    if (ruleRow.ask)
        return FORT_RULE_FILTER_ACTION_ASK;

    return ruleRow.blocked ? FORT_RULE_FILTER_ACTION_BLOCK : FORT_RULE_FILTER_ACTION_ALLOW;
}

QString ruleStateIconPath(const RuleRow &ruleRow)
{
    return ActionRange::actionTypeIconPath(ruleActionTypeId(ruleRow));
}

}

RuleListModel::RuleListModel(QObject *parent) : FtsTableSqlModel(parent)
{
    initialize();
}

SqliteDb *RuleListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void RuleListModel::initialize()
{
    auto confRuleManager = Fort::confRuleManager();
    auto confTimePeriodManager = Fort::confTimePeriodManager();

    connect(confRuleManager, &ConfRuleManager::ruleAdded, this, &TableItemModel::reset);
    connect(confRuleManager, &ConfRuleManager::ruleRemoved, this, &TableItemModel::reset);
    connect(confRuleManager, &ConfRuleManager::ruleUpdated, this, &TableItemModel::refresh);

    // The Time Periods' names are shown, the deleted Time Period is cleared from the Rules
    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodRemoved, this,
            &TableItemModel::refresh);
    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodUpdated, this,
            &TableItemModel::refresh);
}

bool RuleListModel::isIndexRoot(const QModelIndex &index)
{
    const quint32 id = index.internalId();
    return isIndexIdRoot(id);
}

bool RuleListModel::isIndexRule(const QModelIndex &index)
{
    const quint32 id = index.internalId();
    return isIndexIdRule(id);
}

Rule::RuleType RuleListModel::indexRuleType(const QModelIndex &index)
{
    const quint32 id = index.internalId();
    return Rule::RuleType(id);
}

QModelIndex RuleListModel::indexRoot(Rule::RuleType ruleType) const
{
    return createIndex(ruleType, 0, ruleType | InternalIdRoot);
}

QModelIndex RuleListModel::index(int row, int column, const QModelIndex &parent) const
{
    quint8 ruleType;
    InternalIdFlag idFlag;

    if (!parent.isValid()) {
        ruleType = row;
        idFlag = InternalIdRoot;
    } else {
        const quint32 id = parent.internalId();
        if (!isIndexIdRoot(id))
            return QModelIndex();

        ruleType = id;
        idFlag = InternalIdRule;
    }

    return createIndex(row, column, ruleType | idFlag);
}

QModelIndex RuleListModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    const quint32 id = child.internalId();
    if (isIndexIdRoot(id))
        return QModelIndex();

    const quint8 ruleType = id;

    return createIndex(ruleType, 0, ruleType | InternalIdRoot);
}

int RuleListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return Rule::RuleTypeCount;

    const quint32 id = parent.internalId();
    if (!isIndexIdRoot(id))
        return 0;

    setSqlRuleType(id);

    return FtsTableSqlModel::rowCount(parent);
}

int RuleListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 4;
}

QVariant RuleListModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant RuleListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const quint32 id = index.internalId();
    if (isIndexIdRoot(id)) {
        return rootData(index, role);
    }

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index, role);

    // Icon
    case Qt::DecorationRole:
        return dataDecoration(index);

    // Enabled
    case EnabledRole:
        return dataEnabled(index);
    }

    return {};
}

QVariant RuleListModel::rootData(const QModelIndex &index, int role) const
{
    if (index.column() > 0)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return ruleTypeNames().value(index.row());

    // Icon
    case Qt::DecorationRole:
        return IconCache::icon(ruleTypeIconPaths().value(index.row()));

    // Font
    case Qt::FontRole:
        return GuiUtil::fontBold();
    }

    return {};
}

QVariant RuleListModel::headerDataDisplay(int section, int role) const
{
    switch (section) {
    case 0:
        return tr("Rule");
    case 1:
        return (role == Qt::ToolTipRole) ? tr("Terminating Rule") : QVariant();
    case 2:
        return (role == Qt::ToolTipRole) ? tr("Time Period") : QVariant();
    case 3:
        return tr("Change Time");
    }
    return {};
}

QVariant RuleListModel::headerDataDecoration(int section) const
{
    switch (section) {
    case 1:
        return IconCache::icon(":/icons/control_end.png");
    case 2:
        return IconCache::icon(":/icons/clock.png");
    }
    return {};
}

QVariant RuleListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const auto &ruleRow = ruleRowAt(index);

    switch (index.column()) {
    case 0:
        return dataDisplayName(ruleRow, role);
    case 1:
        return dataDisplayTerminate(ruleRow, role);
    case 2:
        return dataDisplayPeriod(ruleRow, role);
    case 3:
        return ruleRow.modTime;
    }

    return {};
}

QVariant RuleListModel::dataDecoration(const QModelIndex &index) const
{
    const auto &ruleRow = ruleRowAt(index);
    if (ruleRow.isNull())
        return {};

    switch (index.column()) {
    case 0:
        return IconCache::icon(ruleStateIconPath(ruleRow));
    case 1:
        return ruleTerminateIcon(ruleRow);
    case 2:
        return rulePeriodIcon(ruleRow);
    }

    return {};
}

QVariant RuleListModel::dataEnabled(const QModelIndex &index) const
{
    const auto &ruleRow = ruleRowAt(index);
    return ruleRow.enabled;
}

void RuleListModel::fillQueryVars(QVariantHash &vars) const
{
    FtsTableSqlModel::fillQueryVars(vars);

    vars.insert(":type", sqlRuleType());
}

Qt::ItemFlags RuleListModel::flagHasChildren(const QModelIndex &index) const
{
    return isIndexRule(index) ? Qt::ItemNeverHasChildren : Qt::NoItemFlags;
}

const RuleRow &RuleListModel::ruleRowAt(const QModelIndex &index) const
{
    setSqlRuleType(index);

    updateRowCache(index.row());

    return m_ruleRow;
}

RuleRow RuleListModel::ruleRowById(int ruleId) const
{
    const QVariantHash vars = { { ":rule_id", ruleId } };

    RuleRow ruleRow;
    updateRuleRow(sqlSelectRules() + " WHERE t.rule_id = :rule_id;", vars, ruleRow);
    return ruleRow;
}

bool RuleListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    return updateRuleRow(sql(), vars, m_ruleRow);
}

bool RuleListModel::updateRuleRow(
        const QString &sql, const QVariantHash &vars, RuleRow &ruleRow) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepareRow(stmt)) {
        ruleRow.invalidate();
        return false;
    }

    ruleRow.ruleId = stmt.columnInt(0);
    ruleRow.enabled = stmt.columnBool(1);
    ruleRow.blocked = stmt.columnBool(2);
    ruleRow.ask = stmt.columnBool(3);
    ruleRow.exclusive = stmt.columnBool(4);
    ruleRow.inlineZones = stmt.columnBool(5);
    ruleRow.terminate = stmt.columnBool(6);
    ruleRow.terminateBlocked = stmt.columnBool(7);
    ruleRow.terminateDrop = stmt.columnBool(8);
    ruleRow.terminateAsk = stmt.columnBool(9);
    ruleRow.terminateAlert = stmt.columnBool(10);
    ruleRow.logAllowedConn = stmt.columnBool(11);
    ruleRow.logBlockedConn = stmt.columnBool(12);
    ruleRow.ruleName = stmt.columnText(13);
    ruleRow.notes = stmt.columnText(14);
    ruleRow.ruleText = stmt.columnText(15);
    ruleRow.ruleType = Rule::RuleType(stmt.columnInt(16));
    ruleRow.zones.accept_mask = stmt.columnUInt(17);
    ruleRow.zones.reject_mask = stmt.columnUInt(18);
    ruleRow.modTime = stmt.columnDateTime(19);
    ruleRow.periodEnabled = stmt.columnBool(20);
    ruleRow.periodId = stmt.columnInt(21);
    ruleRow.trayMenu = stmt.columnBool(22);

    return true;
}

QString RuleListModel::sqlBase() const
{
    return sqlSelectRules() + " WHERE rule_type = :type";
}

QString RuleListModel::sqlSelectRules()
{
    return "SELECT"
           "    t.rule_id,"
           "    t.enabled,"
           "    t.blocked,"
           "    t.ask,"
           "    t.exclusive,"
           "    t.inline_zones,"
           "    t.terminate,"
           "    t.term_blocked,"
           "    t.term_drop,"
           "    t.term_ask,"
           "    t.term_alert,"
           "    t.log_allowed_conn,"
           "    t.log_blocked_conn,"
           "    t.name,"
           "    t.notes,"
           "    t.rule_text,"
           "    t.rule_type,"
           "    t.accept_zones,"
           "    t.reject_zones,"
           "    t.mod_time,"
           "    t.period_enabled,"
           "    t.period_id,"
           "    (menu.rule_id IS NOT NULL) AS tray_menu"
           "  FROM rule t"
           "  LEFT JOIN rule_menu menu ON menu.rule_id = t.rule_id";
}

QString RuleListModel::sqlWhereRegexp() const
{
    return " AND " + regexpFilterColumns();
}

QString RuleListModel::sqlWhereFts() const
{
    return " AND t.rule_id IN ( SELECT rowid FROM rule_fts(:match) )";
}

QString RuleListModel::sqlOrderColumn() const
{
    return "rule_type, lower(name)";
}

const QStringList &RuleListModel::regexpColumns() const
{
    static const QStringList g_regexpColumns = { "t.name", "t.notes" };

    return g_regexpColumns;
}

void RuleListModel::setSqlRuleType(qint8 v) const
{
    if (m_sqlRuleType == v)
        return;

    m_sqlRuleType = v;

    invalidateRowCache();
}

void RuleListModel::setSqlRuleType(const QModelIndex &index) const
{
    setSqlRuleType(index.internalId());
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

QStringList RuleListModel::ruleTypeIconPaths()
{
    static QStringList ruleTypeIcons = {
        ":/icons/script.png",
        ":/icons/script_code.png",
        ":/icons/script_code_red.png",
        ":/icons/books.png",
    };

    return ruleTypeIcons;
}
