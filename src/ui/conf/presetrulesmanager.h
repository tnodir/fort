#ifndef PRESETRULESMANAGER_H
#define PRESETRULESMANAGER_H

#include <QHash>
#include <QSet>
#include <QVariant>

#include <sqlite/sqlite_types.h>

#include <conf/rule.h>

class ConfRuleManager;

// Syncs the default Preset Rules of conf/rule/presets.json with the conf by their codes
// (rule.preset_code): the existing ones are updated to the file's ones (the user's edits are lost,
// except the Enabled and Tray Menu flags), the new ones are added, the ones removed from the file
// become the user's ones (their codes are cleared).
// The file's version (YYYYMMDDNN) synced last is kept in the .ini
class PresetRulesManager
{
public:
    enum SyncMode : qint8 {
        SyncUpdate = 0, // add only the ones new since the version synced last
        SyncRestore, // also add the ones deleted by the user (or not added yet)
    };

    explicit PresetRulesManager(ConfRuleManager *confRuleManager);

    bool changed() const { return m_changed; }

    bool sync(SyncMode mode);

    static QVariantMap loadPresetsFile();

    // The update keeps the user's Enabled and Tray Menu flags
    static void keepUserFlags(Rule &rule, const Rule &oldRule);

private:
    SqliteDb *sqliteDb() const;

    bool isSyncNeeded(SyncMode mode, const QString &version) const;

    bool loadRuleIds();

    bool syncPresets(const QVariantList &presets, SyncMode mode);
    bool syncPreset(const QString &code, const QVariantMap &preset, SyncMode mode);
    bool canAddPreset(const QVariantMap &preset, SyncMode mode) const;
    bool addPreset(const QString &code, const QVariantMap &preset);
    bool updatePreset(quint16 ruleId, const QVariantMap &preset);

    bool clearObsoletePresets(const QSet<QString> &codes);

    bool saveRule(Rule &rule);
    bool saveRuleCode(quint16 ruleId, const QString &code);

    Rule presetRule(const QVariantMap &preset) const;

private:
    bool m_changed = false; // a rule saved or its code cleared

    ConfRuleManager *m_confRuleManager = nullptr;

    QString m_syncedVersion;

    QHash<QString, quint16> m_ruleIds; // by the presets' codes
};

#endif // PRESETRULESMANAGER_H
