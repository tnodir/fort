#include "presetrulesmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitestmt.h>

#include <fortglobal.h>
#include <fortsettings.h>
#include <util/json/jsonutil.h>

#include "confrulemanager.h"

namespace {

const QLoggingCategory LC("confRule");

const char *const sqlSelectPresetRuleIds = "SELECT preset_code, rule_id FROM rule"
                                           "  WHERE preset_code IS NOT NULL;";

const char *const sqlUpdateRulePresetCode = "UPDATE rule SET preset_code = ?2 WHERE rule_id = ?1;";

QString presetCode(const QVariantMap &preset)
{
    return preset.value("code").toString();
}

}

PresetRulesManager::PresetRulesManager(ConfRuleManager *confRuleManager) :
    m_confRuleManager(confRuleManager)
{
}

bool PresetRulesManager::sync(SyncMode mode)
{
    const auto presetsFile = loadPresetsFile();
    const auto version = presetsFile.value("version").toString();
    const auto presets = presetsFile.value("rules").toList();

    if (presets.isEmpty() || !loadRuleIds())
        return false;

    m_syncedVersion = Fort::settings()->rulePresetVersion();

    if (!isSyncNeeded(mode, version))
        return true;

    if (!syncPresets(presets, mode))
        return false;

    Fort::settings()->setRulePresetVersion(version);

    return true;
}

QVariantMap PresetRulesManager::loadPresetsFile()
{
    QString errorString;
    const auto presetsFile = JsonUtil::fileToVariant(":/rule/presets.json", errorString).toMap();
    if (!errorString.isEmpty()) {
        qCWarning(LC) << "Preset Rules: JSON error:" << errorString;
        return {};
    }

    return presetsFile;
}

void PresetRulesManager::keepUserFlags(Rule &rule, const Rule &oldRule)
{
    rule.enabled = oldRule.enabled;
    rule.trayMenu = oldRule.trayMenu;
}

SqliteDb *PresetRulesManager::sqliteDb() const
{
    return m_confRuleManager->sqliteDb();
}

bool PresetRulesManager::isSyncNeeded(SyncMode mode, const QString &version) const
{
    if (mode == SyncRestore)
        return true;

    // Not set up yet (an old profile): only by the user; else the .ini was reset
    if (m_syncedVersion.isEmpty()) {
        return !m_ruleIds.isEmpty();
    }

    return version > m_syncedVersion; // a newer file
}

bool PresetRulesManager::loadRuleIds()
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectPresetRuleIds).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const QString code = stmt.columnText(0);
        const quint16 ruleId = stmt.columnInt(1);

        m_ruleIds.insert(code, ruleId);
    }

    return true;
}

bool PresetRulesManager::syncPresets(const QVariantList &presets, SyncMode mode)
{
    QSet<QString> codes;
    for (const auto &presetVar : presets) {
        const auto preset = presetVar.toMap();
        const QString code = presetCode(preset);
        codes.insert(code);

        if (!syncPreset(code, preset, mode)) {
            qCWarning(LC) << "Preset Rules: Sync error:" << code;
            return false;
        }
    }

    return clearObsoletePresets(codes);
}

bool PresetRulesManager::syncPreset(
        const QString &code, const QVariantMap &preset, SyncMode mode)
{
    const quint16 ruleId = m_ruleIds.value(code);
    if (ruleId == 0) {
        return !canAddPreset(preset, mode) || addPreset(code, preset);
    }

    return updatePreset(ruleId, preset);
}

bool PresetRulesManager::canAddPreset(const QVariantMap &preset, SyncMode mode) const
{
    if (mode == SyncRestore)
        return true;

    // The .ini was reset: maybe deleted by the user
    if (m_syncedVersion.isEmpty())
        return false;

    // Else deleted by the user
    return preset.value("since").toString() > m_syncedVersion;
}

bool PresetRulesManager::addPreset(const QString &code, const QVariantMap &preset)
{
    Rule rule = presetRule(preset);

    if (!saveRule(rule) || !saveRuleCode(rule.ruleId, code))
        return false;

    m_ruleIds.insert(code, rule.ruleId);

    return true;
}

bool PresetRulesManager::updatePreset(quint16 ruleId, const QVariantMap &preset)
{
    Rule rule;
    if (!m_confRuleManager->loadRuleById(ruleId, rule))
        return false;

    // A sub-preset deleted by the user is skipped
    Rule newRule = presetRule(preset);
    newRule.ruleId = rule.ruleId;
    keepUserFlags(newRule, rule);

    if (newRule.isContentEqual(rule))
        return true;

    return saveRule(newRule);
}

// The ones removed from the file are the user's ones now
bool PresetRulesManager::clearObsoletePresets(const QSet<QString> &codes)
{
    for (auto it = m_ruleIds.constBegin(); it != m_ruleIds.constEnd(); ++it) {
        if (codes.contains(it.key()))
            continue;

        if (!saveRuleCode(it.value(), /*code=*/ {})) {
            qCWarning(LC) << "Preset Rules: Clear code error:" << it.key();
            return false;
        }
    }

    return true;
}

bool PresetRulesManager::saveRule(Rule &rule)
{
    bool isNew = false;
    bool isTrayMenuUpdated = false;

    if (!m_confRuleManager->doAddOrUpdateRule(rule, isNew, isTrayMenuUpdated))
        return false;

    m_changed = true;

    return true;
}

bool PresetRulesManager::saveRuleCode(quint16 ruleId, const QString &code)
{
    const QVariantList vars = { ruleId, DbVar::nullable(code) };

    if (!DbQuery(sqliteDb()).sql(sqlUpdateRulePresetCode).vars(vars).executeOk())
        return false;

    m_changed = true;

    return true;
}

Rule PresetRulesManager::presetRule(const QVariantMap &preset) const
{
    Rule rule;
    rule.blocked = preset.value("blocked").toBool();
    rule.terminate = preset.value("terminate").toBool();
    rule.ruleSetEdited = true;
    rule.ruleType = Rule::PresetRule;
    rule.ruleName = preset.value("name").toString();
    rule.notes = preset.value("notes").toString();
    rule.ruleText = preset.value("text").toStringList().join('\n');

    const auto presetCodes = preset.value("presets").toStringList();
    for (const auto &code : presetCodes) {
        const quint16 ruleId = m_ruleIds.value(code);
        if (ruleId != 0) {
            rule.ruleSet.append(ruleId);
        }
    }

    return rule;
}
