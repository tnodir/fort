#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <QColor>
#include <QHash>
#include <QSettings>

class EnvManager;
class FirewallConf;

class FortSettings : public QObject
{
    Q_OBJECT

    friend class IniOptions;

public:
    explicit FortSettings(QObject *parent = nullptr);

    int iniVersion() const { return iniInt("base/version", appVersion()); }
    void setIniVersion(int v) { setIniValue("base/version", v); }

    QString language() const { return iniText("base/language", defaultLanguage()); }

    QString passwordHash() const { return iniText("base/passwordHash"); }
    void setPasswordHash(const QString &v) { setIniValue("base/passwordHash", v); }

    bool noCache() const { return m_noCache; }
    bool isService() const { return m_isService; }
    bool hasService() const { return m_hasService; }
    void setHasService(bool v) { m_hasService = v; }

    bool isServiceClient() const { return hasService() && !isService(); }

    QString defaultLanguage() const { return m_defaultLanguage; }

    QString profilePath() const { return m_profilePath; }

    QString confFilePath() const;

    QString statPath() const { return m_statPath; }
    QString statFilePath() const;

    QString logsPath() const { return m_logsPath; }

    QString cachePath() const { return m_cachePath; }
    QString cacheFilePath() const;

    bool isWindowControl() const { return m_isWindowControl; }
    QString controlCommand() const { return m_controlCommand; }

    const QStringList &args() const { return m_args; }

    const QStringList &appArguments() const { return m_appArguments; }

    bool passwordChecked() const { return m_passwordChecked; }
    int passwordUnlockType() const { return m_passwordUnlockType; }

    bool hasPassword() const { return !passwordHash().isEmpty(); }
    void setPassword(const QString &password);
    bool checkPassword(const QString &password) const;

    bool isPasswordRequired();
    void setPasswordChecked(bool checked, int unlockType = 0);
    void resetCheckedPassword(int unlockType = 0);

    bool confMigrated() const;
    bool confCanMigrate(QString &viaVersion) const;

    bool hasError() const;
    QString errorMessage() const;

signals:
    void passwordCheckedChanged();

public slots:
    void setupGlobal();
    void initialize(const QStringList &args, EnvManager *envManager = nullptr);

    void readConfIni(FirewallConf &conf) const;
    void writeConfIni(const FirewallConf &conf);

    void clearCache();

private:
    void processArguments(const QStringList &args);
    void setupPaths(EnvManager *envManager);
    QString defaultProfilePath() const;

    void setupIni();

    void migrateIniOnStartup();
    void migrateIniOnWrite();

    bool iniBool(const QString &key, bool defaultValue = false) const;
    int iniInt(const QString &key, int defaultValue = 0) const;
    uint iniUInt(const QString &key, int defaultValue = 0) const;
    qreal iniReal(const QString &key, qreal defaultValue = 0) const;
    QString iniText(const QString &key, const QString &defaultValue = QString()) const;
    QStringList iniList(const QString &key) const;
    QVariantMap iniMap(const QString &key) const;
    QByteArray iniByteArray(const QString &key) const;

    QVariant iniValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setIniValue(
            const QString &key, const QVariant &value, const QVariant &defaultValue = QVariant());

    QVariant cacheValue(const QString &key) const;
    void setCacheValue(const QString &key, const QVariant &value) const;

    void removeIniKey(const QString &key);

    QStringList iniChildKeys(const QString &prefix) const;

    void iniSync();

    static int appVersion();

private:
    uint m_iniExists : 1;
    uint m_noCache : 1;
    uint m_isService : 1;
    uint m_hasService : 1;
    uint m_isWindowControl : 1;

    uint m_passwordChecked : 1;
    uint m_passwordUnlockType : 3;

    QString m_defaultLanguage;
    QString m_profilePath;
    QString m_statPath;
    QString m_logsPath;
    QString m_cachePath;
    QString m_controlCommand;
    QStringList m_args;

    QStringList m_appArguments;

    QSettings *m_ini = nullptr;

    mutable QHash<QString, QVariant> m_cache;
};

#endif // FORTSETTINGS_H
