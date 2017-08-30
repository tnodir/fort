#include "fortsettings.h"

#include <QCommandLineParser>
#include <QJsonDocument>
#include <QSettings>
#include <QStandardPaths>

#include "conf/firewallconf.h"
#include "util/fileutil.h"

FortSettings::FortSettings(const QStringList &args,
                           QObject *parent) :
    QObject(parent)
{
    processArguments(args);
    setupIni();
}

void FortSettings::processArguments(const QStringList &args)
{
    const QCommandLineOption profileOption("profile", "Directory to store settings.");

    QCommandLineParser parser;
    parser.addOption(profileOption);
    parser.process(args);

    m_profilePath = parser.value(profileOption);
    if (m_profilePath.isEmpty()) {
        m_profilePath = QStandardPaths::writableLocation(
                    QStandardPaths::AppConfigLocation);
    }
    m_profilePath = FileUtil::absolutePath(m_profilePath);

    const QLatin1Char slash('/');
    if (!m_profilePath.endsWith(slash))
        m_profilePath += slash;
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

QString FortSettings::confFilePath() const
{
    return m_profilePath + QLatin1String("FortFirewall.conf");
}

QString FortSettings::confBackupFilePath() const
{
    return confFilePath() + QLatin1String(".backup");
}

bool FortSettings::readConf(FirewallConf &conf) const
{
    const QString filePath = confFilePath();
    const QString backupFilePath = confBackupFilePath();

    return tryToReadConf(conf, filePath)
            || tryToReadConf(conf, backupFilePath);
}

bool FortSettings::tryToReadConf(FirewallConf &conf, const QString &filePath) const
{
    const QString text = FileUtil::readFile(filePath);

    QJsonParseError jsonParseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(
                text.toUtf8(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError)
        return false;

    conf.fromVariant(jsonDoc.toVariant());

    return readConfFlags(conf);
}

bool FortSettings::writeConf(const FirewallConf &conf)
{
    const QString filePath = confFilePath();
    const QString backupFilePath = confBackupFilePath();

    if (FileUtil::fileExists(backupFilePath)
            && !FileUtil::renameFile(backupFilePath, filePath))
        return false;

    return tryToWriteConf(conf, backupFilePath)
            && FileUtil::renameFile(backupFilePath, filePath);
}

bool FortSettings::tryToWriteConf(const FirewallConf &conf, const QString &filePath)
{
    const QJsonDocument jsonDoc = QJsonDocument::fromVariant(
                conf.toVariant());

    const QByteArray data = jsonDoc.toJson(QJsonDocument::Indented);

    if (FileUtil::writeFileData(filePath, data))
        return false;

    return writeConfFlags(conf);
}

bool FortSettings::readConfFlags(FirewallConf &conf) const
{
    m_ini->beginGroup("confFlags");
    conf.setFilterDisabled(iniBool("filterDisabled"));
    conf.setIpIncludeAll(iniBool("ipIncludeAll"));
    conf.setIpExcludeAll(iniBool("ipExcludeAll"));
    conf.setAppLogBlocked(iniBool("appLogBlocked", true));
    conf.setAppBlockAll(iniBool("appBlockAll", true));
    conf.setAppAllowAll(iniBool("appAllowAll"));
    conf.setAppGroupBits(iniUInt("appGroupBits", 0xFFFF));
    m_ini->endGroup();

    return true;
}

bool FortSettings::writeConfFlags(const FirewallConf &conf)
{
    m_ini->beginGroup("confFlags");
    setIniValue("filterDisabled", conf.filterDisabled());
    setIniValue("ipIncludeAll", conf.ipIncludeAll());
    setIniValue("ipExcludeAll", conf.ipExcludeAll());
    setIniValue("appLogBlocked", conf.appLogBlocked());
    setIniValue("appBlockAll", conf.appBlockAll());
    setIniValue("appAllowAll", conf.appAllowAll());
    setIniValue("appGroupBits", conf.appGroupBits());
    m_ini->endGroup();

    m_ini->sync();

    return m_ini->status() != QSettings::NoError;
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
