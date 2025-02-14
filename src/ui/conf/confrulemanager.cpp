#include "confrulemanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <driver/drivermanager.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confRule");

#define SELECT_RULE_FIELDS                                                                         \
    "    t.rule_id,"                                                                               \
    "    t.enabled,"                                                                               \
    "    t.blocked,"                                                                               \
    "    t.exclusive,"                                                                             \
    "    t.terminate,"                                                                             \
    "    t.term_blocked,"                                                                          \
    "    t.rule_text,"                                                                             \
    "    t.rule_type,"                                                                             \
    "    t.accept_zones,"                                                                          \
    "    t.reject_zones"

const char *const sqlSelectRules = "SELECT" SELECT_RULE_FIELDS "  FROM rule t"
                                   "  ORDER BY t.rule_id;";

const char *const sqlSelectRuleSets = "SELECT * FROM ("
                                      "  SELECT t.rule_id, t.sub_rule_id, t.order_index"
                                      "    FROM rule_set t"
                                      "  UNION ALL" // Global Before App Rules
                                      "  SELECT ?1, t.rule_id, t.rule_id + 99999 AS order_index"
                                      "    FROM rule t"
                                      "    WHERE t.rule_type = ?2"
                                      "  UNION ALL" // Global After App Rules
                                      "  SELECT ?3, t.rule_id, t.rule_id + 99999 AS order_index"
                                      "    FROM rule t"
                                      "    WHERE t.rule_type = ?4"
                                      ") ORDER BY rule_id, order_index;";

const char *const sqlSelectMaxRuleId = "SELECT MAX(rule_id) FROM rule;";

const char *const sqlSelectGlobMinRuleIdByType = "SELECT MIN(t.rule_id) FROM rule t"
                                                 "  WHERE t.rule_type = ?1;";

const char *const sqlInsertRule =
        "INSERT INTO rule(rule_id, enabled, blocked, exclusive,"
        "    terminate, term_blocked, name, notes, rule_text, rule_type,"
        "    accept_zones, reject_zones, mod_time)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13);";

const char *const sqlUpdateRule = "UPDATE rule"
                                  "  SET enabled = ?2, blocked = ?3, exclusive = ?4,"
                                  "    terminate = ?5, term_blocked = ?6, name = ?7,"
                                  "    notes = ?8, rule_text = ?9, rule_type = ?10,"
                                  "    accept_zones = ?11, reject_zones = ?12, mod_time = ?13"
                                  "  WHERE rule_id = ?1;";

const char *const sqlSelectRuleNameById = "SELECT name FROM rule WHERE rule_id = ?1;";

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

bool driverWriteRules(ConfBuffer &confBuf, bool onlyFlags = false)
{
    if (confBuf.hasError()) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeRules(confBuf.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

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

QString ConfRuleManager::ruleNameById(int ruleId)
{
    return DbQuery(sqliteDb()).sql(sqlSelectRuleNameById).vars({ ruleId }).execute().toString();
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
    }

    if (ok) {
        const QVariantList vars = {
            DbVar::nullable(rule.ruleId),
            rule.enabled,
            rule.blocked,
            rule.exclusive,
            rule.terminate,
            rule.terminateBlocked,
            rule.ruleName,
            rule.notes,
            rule.ruleText,
            rule.ruleType,
            rule.acceptZones,
            rule.rejectZones,
            DateUtil::now(),
        };

        DbQuery(sqliteDb(), &ok).sql(isNew ? sqlInsertRule : sqlUpdateRule).vars(vars).executeOk();

        saveRuleSet(rule);
    }

    commitTransaction(ok);

    if (!ok)
        return false;

    updateDriverRules();

    if (isNew) {
        emit ruleAdded();
    } else {
        emit ruleUpdated(rule.ruleId);
    }

    return true;
}

bool ConfRuleManager::deleteRule(int ruleId)
{
    bool ok = false;
    int appRulesCount = 0;

    beginTransaction();

    DbQuery(sqliteDb(), &ok).sql(sqlDeleteRule).vars({ ruleId }).executeOk();
    if (ok) {
        const QVariantList vars = { ruleId };

        // Delete the App Rule from Programs
        DbQuery(sqliteDb()).sql(sqlDeleteAppRule).vars(vars).executeOk();

        appRulesCount = sqliteDb()->changes();

        // Delete the Preset Rule from Rules
        DbQuery(sqliteDb()).sql(sqlDeleteRuleSet).vars(vars).executeOk();
        DbQuery(sqliteDb()).sql(sqlDeleteRuleSetSub).vars(vars).executeOk();
    }

    commitTransaction(ok);

    if (ok) {
        updateDriverRules();

        emit ruleRemoved(ruleId, appRulesCount);
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
        emit ruleUpdated(ruleId);
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
        emit ruleUpdated(ruleId);

        updateDriverRuleFlag(ruleId, enabled);
    }

    return ok;
}

bool ConfRuleManager::walkRules(
        WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const
{
    bool ok = false;

    sqliteDb()->beginTransaction();

    wra.maxRuleId = DbQuery(sqliteDb()).sql(sqlSelectMaxRuleId).execute().toInt();

    const bool hasGlobPreRule = DbQuery(sqliteDb())
                                        .sql(sqlSelectGlobMinRuleIdByType)
                                        .vars({ Rule::GlobalBeforeAppsRule })
                                        .execute()
                                        .toBool();
    if (hasGlobPreRule) {
        wra.globPreRuleId = ++wra.maxRuleId;
    }

    const bool hasGlobPostRule = DbQuery(sqliteDb())
                                         .sql(sqlSelectGlobMinRuleIdByType)
                                         .vars({ Rule::GlobalAfterAppsRule })
                                         .execute()
                                         .toBool();
    if (hasGlobPostRule) {
        wra.globPostRuleId = ++wra.maxRuleId;
    }

    walkRulesMap(wra);

    ok = walkRulesLoop(func);

    sqliteDb()->commitTransaction();

    if (ok && hasGlobPreRule) {
        ok = walkGlobalRule(func, wra.globPreRuleId);
    }

    if (ok && hasGlobPostRule) {
        ok = walkGlobalRule(func, wra.globPostRuleId);
    }

    return ok;
}

void ConfRuleManager::walkRulesMapByStmt(WalkRulesArgs &wra, SqliteStmt &stmt)
{
    int prevRuleId = 0;
    int prevIndex = 0;

    int index = 0;
    for (;;) {
        const bool isStepRow = (stmt.step() == SqliteStmt::StepRow);

        const int ruleId = stmt.columnInt(0);
        const int subRuleId = stmt.columnInt(1);

        if (prevRuleId != ruleId) {
            const RuleSetInfo ruleSetInfo = {
                .index = quint32(prevIndex),
                .count = quint8(index - prevIndex),
            };

            wra.ruleSetMap.insert(prevRuleId, ruleSetInfo);

            prevRuleId = ruleId;
            prevIndex = index;
        }

        if (!isStepRow)
            break;

        wra.ruleSetIds.append(subRuleId);

        ++index;
    }
}

void ConfRuleManager::walkRulesMap(WalkRulesArgs &wra) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb())
                    .sql(sqlSelectRuleSets)
                    .vars({
                            wra.globPreRuleId,
                            Rule::GlobalBeforeAppsRule,
                            wra.globPostRuleId,
                            Rule::GlobalAfterAppsRule,
                    })
                    .prepare(stmt))
        return;

    walkRulesMapByStmt(wra, stmt);
}

bool ConfRuleManager::walkRulesLoop(const std::function<walkRulesCallback> &func) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectRules).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        Rule rule;
        fillRule(rule, stmt);

        if (!func(rule))
            return false;
    }

    return true;
}

bool ConfRuleManager::walkGlobalRule(
        const std::function<walkRulesCallback> &func, quint16 ruleId) const
{
    if (ruleId == 0)
        return true;

    Rule rule;
    rule.ruleId = ruleId;

    return func(rule);
}

void ConfRuleManager::fillRule(Rule &rule, const SqliteStmt &stmt)
{
    rule.ruleId = stmt.columnInt(0);
    rule.enabled = stmt.columnBool(1);
    rule.blocked = stmt.columnBool(2);
    rule.exclusive = stmt.columnBool(3);
    rule.terminate = stmt.columnBool(4);
    rule.terminateBlocked = stmt.columnBool(5);
    rule.ruleText = stmt.columnText(6);
    rule.ruleType = Rule::RuleType(stmt.columnInt(7));
    rule.acceptZones = stmt.columnUInt64(8);
    rule.rejectZones = stmt.columnUInt64(9);
}

void ConfRuleManager::updateDriverRules()
{
    ConfBuffer confBuf;

    confBuf.writeRules(*this);

    driverWriteRules(confBuf);
}

bool ConfRuleManager::updateDriverRuleFlag(int ruleId, bool enabled)
{
    ConfBuffer confBuf;

    confBuf.writeRuleFlag(ruleId, enabled);

    return driverWriteRules(confBuf, /*onlyFlags=*/true);
}

bool ConfRuleManager::beginTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void ConfRuleManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
