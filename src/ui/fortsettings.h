#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <util/ini/settings.h>

class EnvManager;
class FirewallConf;
class IniOptions;

class FortSettings : public Settings
{
    Q_OBJECT

public:
    enum UnlockType : qint8 {
        UnlockDisabled = 0,
        UnlockTillSessionLock,
        UnlockTillAppExit,
    };

    explicit FortSettings(QObject *parent = nullptr);

    static QString passwordHashKey() { return "base/passwordHash"; }

    QString passwordHash() const { return iniText(passwordHashKey()); }
    void setPasswordHash(const QString &v) { setIniValue(passwordHashKey(), v); }

    bool isDefaultProfilePath() const { return m_isDefaultProfilePath; }
    bool noCache() const { return m_noCache; }

    bool isService() const { return m_isService; }
    bool hasService() const { return m_hasService; }

    bool isMaster() const { return !hasService() || isService(); }
    bool hasMasterAdmin() const { return hasService() || isUserAdmin(); }

    bool isUserAdmin() const { return m_isUserAdmin; }

    QString defaultLanguage() const { return m_defaultLanguage; }

    QString profilePath() const { return m_profilePath; }

    QString confFilePath() const;

    QString statPath() const { return m_statPath; }
    QString statFilePath() const;
    QString statBlockFilePath() const;

    QString cachePath() const { return m_cachePath; }
    QString cacheFilePath() const;

    QString userPath() const { return m_userPath; }

    QString logsPath() const { return m_logsPath; }

    QString controlCommand() const { return m_controlCommand; }

    const QStringList &args() const { return m_args; }

    const QStringList &appArguments() const { return m_appArguments; }

    bool passwordChecked() const { return m_passwordChecked; }
    int passwordUnlockType() const { return m_passwordUnlockType; }

    bool hasPassword() const { return !passwordHash().isEmpty(); }
    void setPassword(const QString &password);
    bool checkPassword(const QString &password) const;

    bool isPasswordRequired();
    void setPasswordChecked(bool checked, int unlockType = UnlockDisabled);
    void resetCheckedPassword(int unlockType = UnlockDisabled);

    void setupGlobal();
    void initialize(const QStringList &args, EnvManager *envManager);

    bool wasMigrated() const;
    bool canMigrate(QString &viaVersion) const;

    static bool isPortable();
    static QString defaultProfilePath(bool isService);

    static QStringList unlockTypeStrings();

signals:
    void passwordCheckedChanged();

public slots:
    void readConfIni(FirewallConf &conf) const;
    void readConfIniOptions(const IniOptions &ini) const;

    void writeConfIni(const FirewallConf &conf);
    void writeConfIniOptions(const IniOptions &ini);

protected:
    void migrateIniOnStartup() override;
    void migrateIniOnWrite() override;

private:
    void processArguments(const QStringList &args);

    void setupPaths(EnvManager *envManager);
    void createPaths();

private:
    uint m_isDefaultProfilePath : 1;
    uint m_noCache : 1;
    uint m_isService : 1;
    uint m_hasService : 1;
    uint m_isUserAdmin : 1;

    uint m_passwordChecked : 1;
    uint m_passwordUnlockType : 3;

    QString m_defaultLanguage;
    QString m_profilePath;
    QString m_statPath;
    QString m_cachePath;
    QString m_userPath;
    QString m_logsPath;
    QString m_controlCommand;
    QStringList m_args;

    QStringList m_appArguments;
};

#endif // FORTSETTINGS_H
