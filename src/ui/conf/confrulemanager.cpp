#include "confrulemanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/rule.h>
#include <driver/drivermanager.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/dbutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confRule");

const char *const sqlInsertRule = "INSERT INTO rule(rule_id, enabled, blocked, exclusive, name,"
                                  "    notes, rule_text, accept_zones, reject_zones, mod_time)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10);";

const char *const sqlUpdateRule = "UPDATE rule"
                                  "  SET enabled = ?2, blocked = ?3, exclusive = ?4,"
                                  "    name = ?5, notes = ?6, rule_text = ?7,"
                                  "    accept_zones = ?8, reject_zones = ?9, mod_time = ?10"
                                  "  WHERE rule_id = ?1;";

const char *const sqlSelectRuleIds = "SELECT rule_id FROM rule ORDER BY rule_id;";

const char *const sqlDeleteRule = "DELETE FROM rule WHERE rule_id = ?1;";

const char *const sqlDeleteAppRule = "DELETE FROM app_rule WHERE rule_id = 1;";

const char *const sqlDeleteSystemRule = "DELETE FROM system_rule WHERE rule_id = 1;";

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
        rule.ruleId = DbUtil::getFreeId(sqliteDb(), sqlSelectRuleIds, ConfUtil::ruleMaxCount(), ok);
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
        rule.acceptZones,
        rule.rejectZones,
        DateUtil::now(),
    };

    if (ok) {
        sqliteDb()->executeEx(isNew ? sqlInsertRule : sqlUpdateRule, vars, 0, &ok);
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

    sqliteDb()->executeEx(sqlDeleteRule, vars, 0, &ok);
    if (ok) {
        // Delete the Rule from App Rules
        sqliteDb()->executeEx(sqlDeleteAppRule, vars, 0, &ok);

        // Delete the Rule from System Rules
        sqliteDb()->executeEx(sqlDeleteSystemRule, vars, 0, &ok);
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

    sqliteDb()->executeEx(sqlUpdateRuleName, vars, 0, &ok);

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

    sqliteDb()->executeEx(sqlUpdateRuleEnabled, vars, 0, &ok);

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
    return sqliteDb()->beginTransaction();
}

void ConfRuleManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
