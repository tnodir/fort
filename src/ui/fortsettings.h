#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <QTimer>

#include <conf/inioptions.h>
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
        Unlock5Minutes,
        Unlock10Minutes,
        Unlock30Minutes,
        Unlock1Hour,
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

    QString initialCurrentPath() const { return m_initialCurrentPath; }

    QString profileLogsPath() const { return profilePath() + "logs/"; }

    QString updatePath() const { return m_updatePath; }

    QString controlCommand() const { return m_controlCommand; }
    bool hasControlCommand() const { return !controlCommand().isEmpty(); }

    const QStringList &args() const { return m_args; }

    IniOptions &iniOpt() { return m_iniOpt; }
    const IniOptions &iniOpt() const { return m_iniOpt; }

    bool passwordTemporaryChecked() const { return m_passwordTemporaryChecked; }
    bool passwordChecked() const { return m_passwordChecked; }
    int passwordUnlockType() const { return m_passwordUnlockType; }

    QString passwordUnlockedTillText() const;

    bool hasPassword() const { return !passwordHash().isEmpty(); }
    void setPassword(const QString &password);
    bool checkPassword(const QString &password) const;

    bool isPasswordRequired() const;

    void setPasswordTemporaryChecked(bool checked);
    void setPasswordChecked(bool checked, UnlockType unlockType = UnlockDisabled);
    void resetCheckedPassword(UnlockType unlockType = UnlockDisabled);

    void setupGlobal();
    void setupGlobalDpi(const QSettings &settings);

    void initialize(const QStringList &args, EnvManager *envManager);

    static bool isPortable();
    static QString defaultProfilePath(bool isService);

    static QStringList unlockTypeStrings();
    static int unlockTypeMinutes(UnlockType unlockType);

    // COMPAT: The App. Groups' enabled bits by their indexes
    quint32 appGroupBits() const { return iniUInt("confFlags/appGroupBits", quint32(-1)); }

    // COMPAT: The Statistics' active period's times, replaced by the Time Period
    bool activePeriodEnabled() const { return iniBool("stat/activePeriodEnabled"); }
    QString activePeriodFrom() const { return iniText("stat/activePeriodFrom"); }
    QString activePeriodTo() const { return iniText("stat/activePeriodTo"); }
    void setActivePeriodId(quint8 v) { saveIniValue("stat/activePeriodId", v); }

signals:
    void passwordCheckedChanged();

public slots:
    void readConfIni(FirewallConf &conf) const;
    void readConfIniOptions(const IniOptions &ini) const;

    void writeConfIni(const FirewallConf &conf, IniOptions &iniOpt);
    void writeConfIniOptions(IniOptions &ini);

protected:
    void migrateIniOnLoad() override;
    void migrateIniOnWrite() override;

private:
    void migrateExplorerIntegration(int version);
    void keepConfDbMigrationValues(int version);
    void removeConfDbMigrationKeys(int version);

    void setupPasswordUnlockTimer();
    void startPasswordUnlockTimer();

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
    uint m_passwordTemporaryChecked : 1 = false;
    uint m_passwordChecked : 1 = false;

    UnlockType m_passwordUnlockType = UnlockDisabled;

    QTimer m_passwordUnlockTimer;

    QString m_defaultLanguage;
    QString m_profilePath;
    QString m_statPath;
    QString m_cachePath;
    QString m_userPath;
    QString m_logsPath;
    QString m_outputPath;
    QString m_initialCurrentPath;
    QString m_updatePath;
    QString m_controlCommand;
    QStringList m_args;

    IniOptions m_iniOpt;
};

#endif // FORTSETTINGS_H
