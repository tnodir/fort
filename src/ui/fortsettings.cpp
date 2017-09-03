#include "fortsettings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QSettings>

#include "conf/addressgroup.h"
#include "conf/firewallconf.h"
#include "util/fileutil.h"

FortSettings::FortSettings(const QStringList &args,
                           QObject *parent) :
    QObject(parent)
{
    processArguments(args);
    setupIni();
}

bool FortSettings::startWithWindows() const
{
    return FileUtil::fileExists(startupShortcutPath());
}

void FortSettings::setStartWithWindows(bool start)
{
    const QString linkPath = startupShortcutPath();
    if (start) {
        FileUtil::linkFile(qApp->applicationFilePath(), linkPath);
    } else {
        FileUtil::removeFile(linkPath);
    }
    emit startWithWindowsChanged();
}

void FortSettings::processArguments(const QStringList &args)
{
    QCommandLineParser parser;

    const QCommandLineOption bootOption(
                "boot", "Block access to network when Fort Firewall is not running.");
    parser.addOption(bootOption);

    const QCommandLineOption profileOption(
                QStringList() << "p" << "profile",
                "Directory to store settings.", "profile");
    parser.addOption(profileOption);

    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(args);

    m_boot = parser.isSet(bootOption);

    m_profilePath = parser.value(profileOption);
    if (m_profilePath.isEmpty()) {
        m_profilePath = FileUtil::appConfigLocation();
    }
    m_profilePath = FileUtil::absolutePath(m_profilePath);

    const QLatin1Char slash('/');
    if (!m_profilePath.endsWith(slash)) {
        m_profilePath += slash;
    }
}

void FortSettings::setupIni()
{
    const QString qrcIniPath(":/FortFirewall.ini");
    const QString iniPath(m_profilePath + "FortFirewall.ini");

    FileUtil::makePath(m_profilePath);

    // Copy default .ini into writable location
    if (!FileUtil::fileExists(iniPath)) {
        const QString text = FileUtil::readFile(qrcIniPath);
        if (!FileUtil::writeFile(iniPath, text)) {
            FileUtil::removeFile(iniPath);
        }
    }

    m_ini = new QSettings(iniPath, QSettings::IniFormat, this);
}

void FortSettings::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

QString FortSettings::confFilePath() const
{
    return m_profilePath + QLatin1String("FortFirewall.conf");
}

QString FortSettings::confBackupFilePath() const
{
    return confFilePath() + QLatin1String(".backup");
}

bool FortSettings::readConf(FirewallConf &conf)
{
    const QString filePath = confFilePath();
    const QString backupFilePath = confBackupFilePath();

    return tryToReadConf(conf, filePath)
            || tryToReadConf(conf, backupFilePath);
}

bool FortSettings::tryToReadConf(FirewallConf &conf, const QString &filePath)
{
    const QByteArray data = FileUtil::readFileData(filePath);

    QJsonParseError jsonParseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(
                data, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        setErrorMessage(jsonParseError.errorString());
        return false;
    }

    conf.fromVariant(jsonDoc.toVariant());

    return readConfFlags(conf);
}

bool FortSettings::writeConf(const FirewallConf &conf)
{
    const QString filePath = confFilePath();
    const QString backupFilePath = confBackupFilePath();

    if (FileUtil::fileExists(backupFilePath)
            && !FileUtil::renameFile(backupFilePath, filePath)) {
        setErrorMessage(tr("Can't rename old backup conf. file"));
        return false;
    }

    if (!tryToWriteConf(conf, backupFilePath))
        return false;

    if (!FileUtil::renameFile(backupFilePath, filePath)) {
        setErrorMessage(tr("Can't rename backup conf. file"));
        return false;
    }

    return true;
}

bool FortSettings::tryToWriteConf(const FirewallConf &conf, const QString &filePath)
{
    const QJsonDocument jsonDoc = QJsonDocument::fromVariant(
                conf.toVariant());

    const QByteArray data = jsonDoc.toJson(QJsonDocument::Indented);

    if (!FileUtil::writeFileData(filePath, data)) {
        setErrorMessage(tr("Can't write conf. file"));
        return false;
    }

    return writeConfFlags(conf);
}

bool FortSettings::readConfFlags(FirewallConf &conf) const
{
    m_ini->beginGroup("confFlags");
    conf.setFilterEnabled(iniBool("filterEnabled", true));
    conf.ipInclude()->setUseAll(iniBool("ipIncludeAll"));
    conf.ipExclude()->setUseAll(iniBool("ipExcludeAll"));
    conf.setAppBlockAll(iniBool("appBlockAll", true));
    conf.setAppAllowAll(iniBool("appAllowAll"));
    conf.setAppGroupBits(iniUInt("appGroupBits", 0xFFFF));
    m_ini->endGroup();

    return true;
}

bool FortSettings::writeConfFlags(const FirewallConf &conf)
{
    m_ini->beginGroup("confFlags");
    setIniValue("filterEnabled", conf.filterEnabled());
    setIniValue("ipIncludeAll", conf.ipInclude()->useAll());
    setIniValue("ipExcludeAll", conf.ipExclude()->useAll());
    setIniValue("appBlockAll", conf.appBlockAll());
    setIniValue("appAllowAll", conf.appAllowAll());
    setIniValue("appGroupBits", conf.appGroupBits());
    m_ini->endGroup();

    m_ini->sync();

    return m_ini->status() == QSettings::NoError;
}

bool FortSettings::iniBool(const QString &key, bool defaultValue) const
{
    return iniValue(key, defaultValue).toBool();
}

int FortSettings::iniInt(const QString &key, int defaultValue) const
{
    return iniValue(key, defaultValue).toInt();
}

int FortSettings::iniUInt(const QString &key, int defaultValue) const
{
    return iniValue(key, defaultValue).toUInt();
}

int FortSettings::iniReal(const QString &key, qreal defaultValue) const
{
    return iniValue(key, defaultValue).toReal();
}

QString FortSettings::iniText(const QString &key, const QString &defaultValue) const
{
    return iniValue(key, defaultValue).toString();
}

QStringList FortSettings::iniList(const QString &key) const
{
    return iniValue(key).toStringList();
}

QVariant FortSettings::iniValue(const QString &key,
                                const QVariant &defaultValue) const
{
    if (key.isEmpty())
        return QVariant();

    return m_ini->value(key, defaultValue);
}

void FortSettings::setIniValue(const QString &key, const QVariant &value,
                               const QVariant &defaultValue)
{
    if (m_ini->value(key, defaultValue) == value)
        return;

    m_ini->setValue(key, value);
    emit iniChanged();
}

QString FortSettings::startupShortcutPath()
{
    return FileUtil::applicationsLocation() + QLatin1Char('\\')
            + "Startup" + QLatin1Char('\\')
            + qApp->applicationName() + ".lnk";
}
