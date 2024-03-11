#include "confrulemanager.h"

#include <QLoggingCategory>

#include <sqlite/dbutil.h>
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

const char *const sqlSelectRuleIds = "SELECT rule_id FROM rule"
                                     "  WHERE rule_id BETWEEN ?1 AND ?2 ORDER BY rule_id;";

const char *const sqlDeleteRule = "DELETE FROM rule WHERE rule_id = ?1;";

const char *const sqlDeleteAppRule = "DELETE FROM app_rule WHERE rule_id = 1;";

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

bool ConfRuleManager::addOrUpdateRule(Rule &rule)
{
    bool ok = true;

    beginTransaction();

    const bool isNew = (rule.ruleId == 0);
    if (isNew) {
        const auto range = Rule::getRuleIdRangeByType(rule.ruleType);

        rule.ruleId = DbUtil(sqliteDb(), &ok)
                              .sql(sqlSelectRuleIds)
                              .vars({ range.minId, range.maxId })
                              .getFreeId(range.minId, range.maxId);
    } else {
        updateDriverRuleFlag(rule.ruleId, rule.enabled);
    }

    const QVariantList vars = {
        rule.ruleId,
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
        DbUtil(sqliteDb(), &ok).sql(isNew ? sqlInsertRule : sqlUpdateRule).vars(vars).executeOk();
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

    const QVariantList vars = { ruleId };

    DbUtil(sqliteDb(), &ok).sql(sqlDeleteRule).vars(vars).executeOk();
    if (ok) {
        // Delete the Rule from App Rules
        DbUtil(sqliteDb(), &ok).sql(sqlDeleteAppRule).vars(vars).executeOk();
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

    DbUtil(sqliteDb(), &ok).sql(sqlUpdateRuleName).vars(vars).executeOk();

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

    DbUtil(sqliteDb(), &ok).sql(sqlUpdateRuleEnabled).vars(vars).executeOk();

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

bool ConfRuleManager::beginTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void ConfRuleManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
