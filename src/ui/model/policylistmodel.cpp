#include "policylistmodel.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <util/ioc/ioccontainer.h>

PolicyListModel::PolicyListModel(Policy::PolicyType policyType, QObject *parent) :
    TableSqlModel(parent), m_policyType(policyType)
{
}

ConfManager *PolicyListModel::confManager() const
{
    return IoC<ConfManager>();
}

SqliteDb *PolicyListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void PolicyListModel::initialize()
{
    connect(confManager(), &ConfManager::confChanged, this, &TableItemModel::reset);
}

int PolicyListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant PolicyListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);

    // Enabled
    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return QVariant();
}

QVariant PolicyListModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();

    const auto policyRow = policyRowAt(row);

    return policyRow.name;
}

QVariant PolicyListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto policyRow = policyRowAt(index.row());
        return policyRow.enabled ? Qt::Checked : Qt::Unchecked;
    }

    return QVariant();
}

bool PolicyListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto policyRow = policyRowAt(index.row());
        return false; // confManager()->updatePolicyEnabled(policyRow.policyId, !policyRow.enabled);
    }

    return false;
}

Qt::ItemFlags PolicyListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

const PolicyRow &PolicyListModel::policyRowAt(int row) const
{
    updateRowCache(row);

    return m_policyRow;
}

bool PolicyListModel::updateTableRow(int row) const
{
    return updatePolicyRow(sql(), { row }, m_policyRow);
}

bool PolicyListModel::updatePolicyRow(
        const QString &sql, const QVariantList &vars, PolicyRow &policyRow) const
{
    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql, vars) && stmt.step() == SqliteStmt::StepRow)) {
        policyRow.invalidate();
        return false;
    }

    policyRow.policyId = stmt.columnInt(0);
    policyRow.policyType = Policy::PolicyType(stmt.columnInt(1));
    policyRow.enabled = stmt.columnBool(2);
    policyRow.name = stmt.columnText(3);

    return true;
}

QString PolicyListModel::sqlBase() const
{
    return "SELECT"
           "    policy_id,"
           "    policy_type,"
           "    enabled,"
           "    name"
           "  FROM policy t";
}

QString PolicyListModel::sqlWhere() const
{
    return QString::fromLatin1(" WHERE t.type = %1").arg(policyType());
}

QString PolicyListModel::sqlOrder() const
{
    return " t.name";
}
