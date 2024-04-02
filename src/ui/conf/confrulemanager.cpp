#include "confrulemanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/rule.h>
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

const char *const sqlSelectMaxRuleId = "SELECT MAX(rule_id) FROM rule;";

const char *const sqlInsertFreeRuleId = "INSERT INTO rule_free(rule_id) VALUES(?1);";

const char *const sqlDeleteFreeRuleId = "DELETE FROM rule_free"
                                        "  WHERE rule_id = ("
                                        "    SELECT MAX(rule_id) FROM rule_free"
                                        "  ) RETURNING rule_id;";

const char *const sqlDeleteFreeRuleIds = "DELETE FROM rule_free WHERE rule_id >= ?1;";

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
    m_confManager = IoCPinned()->setUpDependency<ConfManager>();
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

bool ConfRuleManager::addOrUpdateRule(Rule &rule)
{
    bool ok = true;

    beginTransaction();

    const bool isNew = (rule.ruleId == 0);
    if (isNew) {
        // Get Rule Id from the free list
        rule.ruleId = getFreeRuleId();
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

        // Put the Rule Id back to the free list
        putFreeRuleId(ruleId);
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
    QByteArray buf;

#if 0
    const int entrySize = confUtil.writeRules(rulesMask, enabledMask, dataSize, rulesData, buf);

    driverWriteRules(confUtil, buf, entrySize);
#endif
}

bool ConfRuleManager::updateDriverRuleFlag(int ruleId, bool enabled)
{
    ConfUtil confUtil;
    QByteArray buf;

#if 0
    const int entrySize = confUtil.writeRuleFlag(ruleId, enabled, buf);

    return driverWriteRules(confUtil, buf, entrySize, /*onlyFlags=*/true);
#endif
    return true;
}

int ConfRuleManager::getFreeRuleId()
{
    return DbQuery(sqliteDb()).sql(sqlDeleteFreeRuleId).execute().toInt();
}

void ConfRuleManager::putFreeRuleId(int ruleId)
{
    const QVariantList vars = { ruleId };

    const int maxRuleId = DbQuery(sqliteDb()).sql(sqlSelectMaxRuleId).execute().toInt();

    if (ruleId < maxRuleId) {
        // Add the Rule Id to free list
        DbQuery(sqliteDb()).sql(sqlInsertFreeRuleId).vars(vars).executeOk();
    } else {
        // Delete outdated free Rule Ids
        DbQuery(sqliteDb()).sql(sqlDeleteFreeRuleIds).vars(vars).executeOk();
    }
}

bool ConfRuleManager::beginTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void ConfRuleManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
