#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <util/ini/settings.h>

QT_FORWARD_DECLARE_CLASS(QCommandLineOption)
QT_FORWARD_DECLARE_CLASS(QCommandLineParser)

class EnvManager;
class FirewallConf;
class IniOptions;

class FortSettings : public Settings
{
    Q_OBJECT

public:
    enum UnlockType : qint8 {
        UnlockDisabled = -1,
        UnlockWindow = 0,
        UnlockSession,
        UnlockApp,
    };

    explicit FortSettings(QObject *parent = nullptr);

    static QString passwordHashKey() { return "base/passwordHash"; }

    QString passwordHash() const { return iniText(passwordHashKey()); }
    void setPasswordHash(const QString &v) { setIniValue(passwordHashKey(), v); }

    bool isDefaultProfilePath() const { return m_isDefaultProfilePath; }
    bool noCache() const { return m_noCache; }
    bool noSplash() const { return m_noSplash; }
    bool forceDebug() const { return m_forceDebug; }
    bool canInstallDriver() const { return m_canInstallDriver; }
    bool canStartService() const { return m_canStartService; }
    bool checkProfileOnline() const { return m_checkProfileOnline; }

    bool isLaunch() const { return m_isLaunch; }

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
    QString statConnFilePath() const;

    QString cachePath() const { return m_cachePath; }
    QString cacheFilePath() const;

    QString userPath() const { return m_userPath; }

    QString logsPath() const { return m_logsPath; }

    QString outputPath() const { return m_outputPath; }

    QString profileLogsPath() const { return profilePath() + "logs/"; }

    QString updatePath() const { return m_updatePath; }

    QString controlCommand() const { return m_controlCommand; }

    const QStringList &args() const { return m_args; }

    bool passwordChecked() const { return m_passwordChecked; }
    int passwordUnlockType() const { return m_passwordUnlockType; }

    QString passwordUnlockedTillText() const;

    bool hasPassword() const { return !passwordHash().isEmpty(); }
    void setPassword(const QString &password);
    bool checkPassword(const QString &password) const;

    bool isPasswordRequired() const;

    void setPasswordChecked(bool checked, UnlockType unlockType = UnlockDisabled);
    void resetCheckedPassword(UnlockType unlockType = UnlockDisabled);

    void setupGlobal();
    void setupGlobalDpi(const QSettings &settings);

    void initialize(const QStringList &args, EnvManager *envManager);

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
    void migrateIniOnLoad() override;
    void migrateIniOnWrite() override;

private:
    void processProfileOption(
            const QCommandLineParser &parser, const QCommandLineOption &profileOption);
    void processStatOption(const QCommandLineParser &parser, const QCommandLineOption &statOption);
    void processCacheOption(
            const QCommandLineParser &parser, const QCommandLineOption &cacheOption);
    void processLogsOption(const QCommandLineParser &parser, const QCommandLineOption &logsOption);
    void processOutputOption(
            const QCommandLineParser &parser, const QCommandLineOption &outputOption);
    void processNoCacheOption(
            const QCommandLineParser &parser, const QCommandLineOption &noCacheOption);
    void processNoSplashOption(
            const QCommandLineParser &parser, const QCommandLineOption &noSplashOption);
    void processLangOption(const QCommandLineParser &parser, const QCommandLineOption &langOption);
    void processLaunchOption(
            const QCommandLineParser &parser, const QCommandLineOption &launchOption);
    void processServiceOption(
            const QCommandLineParser &parser, const QCommandLineOption &serviceOption);
    void processControlOption(
            const QCommandLineParser &parser, const QCommandLineOption &controlOption);
    void processOtherOptions(const QCommandLineParser &parser);
    void processArguments(const QStringList &args);

    void setupPaths(EnvManager *envManager);
    void createPaths();

private:
    uint m_isDefaultProfilePath : 1 = false;
    uint m_noCache : 1 = false;
    uint m_noSplash : 1 = false;
    uint m_forceDebug : 1 = false;
    uint m_canInstallDriver : 1 = false;
    uint m_canStartService : 1 = false;
    uint m_checkProfileOnline : 1 = false;
    uint m_isLaunch : 1 = false;
    uint m_isService : 1 = false;
    uint m_hasService : 1 = false;
    uint m_isUserAdmin : 1 = false;
    uint m_passwordChecked : 1 = false;

    UnlockType m_passwordUnlockType = UnlockDisabled;

    QString m_defaultLanguage;
    QString m_profilePath;
    QString m_statPath;
    QString m_cachePath;
    QString m_userPath;
    QString m_logsPath;
    QString m_outputPath;
    QString m_updatePath;
    QString m_controlCommand;
    QStringList m_args;
};

#endif // FORTSETTINGS_H
