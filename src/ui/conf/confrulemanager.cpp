#include "confrulemanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <driver/drivermanager.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confRule");

const char *const sqlInsertRule = "INSERT INTO rule(rule_id, enabled, blocked, exclusive,"
                                  "    name, notes, rule_text, rule_type,"
                                  "    accept_zones, reject_zones, mod_time)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11);";

const char *const sqlUpdateRule = "UPDATE rule"
                                  "  SET enabled = ?2, blocked = ?3, exclusive = ?4,"
                                  "    name = ?5, notes = ?6, rule_text = ?7, rule_type = ?8,"
                                  "    accept_zones = ?9, reject_zones = ?10, mod_time = ?11"
                                  "  WHERE rule_id = ?1;";

const char *const sqlSelectRuleIds = "SELECT rule_id FROM rule"
                                     "  WHERE rule_id < ?1 ORDER BY rule_id;";

const char *const sqlDeleteRule = "DELETE FROM rule WHERE rule_id = ?1;";

const char *const sqlDeleteAppRule = "UPDATE app"
                                     "  SET rule_id = NULL"
                                     "  WHERE rule_id = ?1;";

const char *const sqlInsertRuleSet = "INSERT INTO rule_set(rule_id, sub_rule_id, order_index)"
                                     "  VALUES(?1, ?2, ?3);";

const char *const sqlDeleteRuleSet = "DELETE FROM rule_set WHERE rule_id = ?1;";

const char *const sqlDeleteRuleSetSub = "DELETE FROM rule_set WHERE sub_rule_id = ?1;";

const char *const sqlSelectRuleSet = "SELECT t.sub_rule_id, r.name"
                                     "  FROM rule_set t"
                                     "  JOIN rule r ON r.rule_id = t.sub_rule_id"
                                     "  WHERE t.rule_id = ?1"
                                     "  ORDER BY t.order_index;";

const char *const sqlSelectRulesCountByType = "SELECT COUNT(*) FROM rule t"
                                              "  WHERE t.rule_type = ?1;";

const char *const sqlSelectRuleSetLoop = "WITH RECURSIVE"
                                         "  parents(rule_id, level) AS ("
                                         "    VALUES(?1, 0)"
                                         "    UNION ALL"
                                         "    SELECT t.rule_id, p.level + 1"
                                         "      FROM rule_set t"
                                         "      JOIN parents p ON p.rule_id = t.sub_rule_id"
                                         "  ),"
                                         "  children(rule_id, level) AS ("
                                         "    VALUES(?2, 0)"
                                         "    UNION ALL"
                                         "    SELECT t.sub_rule_id, c.level + 1"
                                         "      FROM rule_set t"
                                         "      JOIN children c ON c.rule_id = t.rule_id"
                                         "  )"
                                         "SELECT * FROM (VALUES ("
                                         "  (SELECT 1"
                                         "    FROM parents p"
                                         "    JOIN children c ON c.rule_id = p.rule_id"
                                         "    LIMIT 1),"
                                         "  ((SELECT MAX(level) FROM parents)"
                                         "    + (SELECT MAX(level) FROM children))"
                                         "));";

const char *const sqlUpdateRuleName = "UPDATE rule SET name = ?2 WHERE rule_id = ?1;";

const char *const sqlUpdateRuleEnabled = "UPDATE rule SET enabled = ?2 WHERE rule_id = ?1;";

bool driverWriteRules(ConfUtil &confUtil, QByteArray &buf, int entrySize, bool onlyFlags = false)
{
    if (entrySize == 0) {
        qCWarning(LC) << "Driver config error:" << confUtil.errorMessage();
        return false;
    }

#if 0
    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeRules(buf, entrySize, onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }
#endif

    return true;
}

}

ConfRuleManager::ConfRuleManager(QObject *parent) : QObject(parent) { }

ConfManager *ConfRuleManager::confManager() const
{
    return m_confManager;
}

SqliteDb *ConfRuleManager::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void ConfRuleManager::setUp()
{
    m_confManager = IoCDependency<ConfManager>();
}

void ConfRuleManager::loadRuleSet(Rule &rule, QStringList &ruleSetNames)
{
    rule.ruleSetEdited = false;
    rule.ruleSet.clear();

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectRuleSet).vars({ rule.ruleId }).prepare(stmt))
        return;

    while (stmt.step() == SqliteStmt::StepRow) {
        const int subRuleId = stmt.columnInt(0);
        const auto subRuleName = stmt.columnText(1);

        rule.ruleSet.append(subRuleId);
        ruleSetNames.append(subRuleName);
    }
}

void ConfRuleManager::saveRuleSet(Rule &rule)
{
    if (!rule.ruleSetEdited)
        return;

    DbQuery(sqliteDb()).sql(sqlDeleteRuleSet).vars({ rule.ruleId }).executeOk();

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlInsertRuleSet).prepare(stmt))
        return;

    stmt.bindInt(1, rule.ruleId);

    int orderIndex = 0;
    for (const auto subRuleId : rule.ruleSet) {
        stmt.bindInt(2, subRuleId);
        stmt.bindInt(3, ++orderIndex);

        stmt.step();
        stmt.reset();
    }
}

int ConfRuleManager::rulesCountByType(Rule::RuleType ruleType)
{
    return DbQuery(sqliteDb()).sql(sqlSelectRulesCountByType).vars({ ruleType }).execute().toInt();
}

bool ConfRuleManager::checkRuleSetValid(int ruleId, int subRuleId, int extraDepth)
{
    const auto list = DbQuery(sqliteDb())
                              .sql(sqlSelectRuleSetLoop)
                              .vars({ ruleId, subRuleId })
                              .execute(2)
                              .toList();

    const int loopId = list[0].toInt();
    const int depth = list[1].toInt();

    return loopId == 0 && (depth + extraDepth) <= ConfUtil::ruleSetDepthMaxCount();
}

bool ConfRuleManager::addOrUpdateRule(Rule &rule)
{
    bool ok = true;

    beginTransaction();

    const bool isNew = (rule.ruleId == 0);
    if (isNew) {
        rule.ruleId = DbQuery(sqliteDb(), &ok)
                              .sql(sqlSelectRuleIds)
                              .vars({ ConfUtil::ruleMaxCount() })
                              .getFreeId(/*maxId=*/ConfUtil::ruleMaxCount() - 1);
    } else {
        updateDriverRuleFlag(rule.ruleId, rule.enabled);
    }

    const QVariantList vars = {
        DbVar::nullable(rule.ruleId),
        rule.enabled,
        rule.blocked,
        rule.exclusive,
        rule.ruleName,
        rule.notes,
        rule.ruleText,
        rule.ruleType,
        rule.acceptZones,
        rule.rejectZones,
        DateUtil::now(),
    };

    if (ok) {
        DbQuery(sqliteDb(), &ok).sql(isNew ? sqlInsertRule : sqlUpdateRule).vars(vars).executeOk();

        saveRuleSet(rule);
    }

    commitTransaction(ok);

    if (!ok)
        return false;

    if (isNew) {
        emit ruleAdded();
    } else {
        emit ruleUpdated();
    }

    return true;
}

bool ConfRuleManager::deleteRule(int ruleId)
{
    bool ok = false;

    beginTransaction();

    DbQuery(sqliteDb(), &ok).sql(sqlDeleteRule).vars({ ruleId }).executeOk();
    if (ok) {
        const QVariantList vars = { ruleId };

        // Delete the App Rule from Programs
        DbQuery(sqliteDb()).sql(sqlDeleteAppRule).vars(vars).executeOk();

        // Delete the Preset Rule from Rules
        DbQuery(sqliteDb()).sql(sqlDeleteRuleSet).vars(vars).executeOk();
        DbQuery(sqliteDb()).sql(sqlDeleteRuleSetSub).vars(vars).executeOk();
    }

    commitTransaction(ok);

    if (ok) {
        emit ruleRemoved(ruleId);
    }

    return ok;
}

bool ConfRuleManager::updateRuleName(int ruleId, const QString &ruleName)
{
    bool ok = false;

    beginTransaction();

    const QVariantList vars = { ruleId, ruleName };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateRuleName).vars(vars).executeOk();

    commitTransaction(ok);

    if (ok) {
        emit ruleUpdated();
    }

    return ok;
}

bool ConfRuleManager::updateRuleEnabled(int ruleId, bool enabled)
{
    bool ok = false;

    beginTransaction();

    const QVariantList vars = { ruleId, enabled };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateRuleEnabled).vars(vars).executeOk();

    commitTransaction(ok);

    if (ok) {
        emit ruleUpdated();

        updateDriverRuleFlag(ruleId, enabled);
    }

    return ok;
}

void ConfRuleManager::updateDriverRules(quint32 rulesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &rulesData)
{
    ConfUtil confUtil;

#if 0
    const int entrySize = confUtil.writeRules(rulesMask, enabledMask, dataSize, rulesData);

    driverWriteRules(confUtil, confUtil.buffer(), entrySize);
#endif
}

bool ConfRuleManager::updateDriverRuleFlag(int ruleId, bool enabled)
{
    ConfUtil confUtil;

#if 0
    const int entrySize = confUtil.writeRuleFlag(ruleId, enabled);

    return driverWriteRules(confUtil, confUtil.buffer(), entrySize, /*onlyFlags=*/true);
#endif
    return true;
}

bool ConfRuleManager::beginTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void ConfRuleManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
