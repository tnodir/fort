#include "confmanagerbase.h"

#include <sqlite/dbquery.h>
#include <sqlite/sqlitestmt.h>

#include <fortglobal.h>

#include "confmanager.h"

ConfManagerBase::ConfManagerBase(QObject *parent) : QObject(parent) { }

SqliteDb *ConfManagerBase::sqliteDb() const
{
    return Fort::confManager()->sqliteDb();
}

void ConfManagerBase::saveRuleSetIds(
        const char *sqlDelete, const char *sqlInsert, qint64 ownerId, const RuleSetList &ruleSet)
{
    DbQuery(sqliteDb()).sql(sqlDelete).vars({ ownerId }).executeOk();

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlInsert).prepare(stmt))
        return;

    stmt.bindInt64(1, ownerId);

    int orderIndex = 0;
    for (const quint16 ruleId : ruleSet) {
        stmt.bindInt(2, ruleId);
        stmt.bindInt(3, ++orderIndex);

        stmt.step();
        stmt.reset();
    }
}

ConfUsages ConfManagerBase::loadUsages(const char *sql, const QVariantList &vars) const
{
    ConfUsages usages;

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepare(stmt))
        return usages;

    while (stmt.step() == SqliteStmt::StepRow) {
        const auto category = ConfUsages::Category(stmt.columnInt(0));

        usages.lists[category].append({ .id = stmt.columnInt64(1), .name = stmt.columnText(2) });
    }

    return usages;
}
