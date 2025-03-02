#include "settings.h"

#include <QLoggingCategory>

#include <fort_version.h>

#include <util/fileutil.h>

namespace {

const QLoggingCategory LC("settings");

}

Settings::Settings(QObject *parent) : QObject(parent) { }

bool Settings::canMigrate(QString &viaVersion) const
{
    int version;
    if (checkIniVersion(version))
        return true;

    // COMPAT: v3.0.0
    if (version < 0x030000 && appVersion() > 0x030000) {
        viaVersion = "3.0.0";
        return false;
    }

    return true;
}

bool Settings::checkIniVersion(int &oldVersion) const
{
    if (!iniExists())
        return true;

    if (!iniVersionSet()) {
        // TODO: COMPAT: Change after v4.1.0 (via v4.0.0) to be 0
        oldVersion = 0x030800;
        return false;
    }

    oldVersion = iniVersion();
    return oldVersion == appVersion();
}

bool Settings::checkIniVersionOnWrite(int &oldVersion) const
{
    if (!iniExists()) {
        oldVersion = appVersion();
        return false;
    }

    return checkIniVersion(oldVersion);
}

void Settings::setupIni(const QString &filePath)
{
    m_iniExists = FileUtil::fileExists(filePath);
    m_ini = new QSettings(filePath, QSettings::IniFormat, this);

    migrateIniOnLoad();
}

void Settings::migrateIniOnWrite()
{
    setIniVersion(appVersion());
}

bool Settings::hasError() const
{
    return m_ini->status() != QSettings::NoError;
}

QString Settings::errorMessage() const
{
    switch (m_ini->status()) {
    case QSettings::AccessError:
        return "Access Error";
    case QSettings::FormatError:
        return "Format Error";
    default:
        return "Unknown";
    }
}

bool Settings::iniBool(const QString &key, bool defaultValue) const
{
    return iniValue(key, defaultValue).toBool();
}

int Settings::iniInt(const QString &key, int defaultValue) const
{
    return iniValue(key, defaultValue).toInt();
}

uint Settings::iniUInt(const QString &key, uint defaultValue) const
{
    return iniValue(key, defaultValue).toUInt();
}

quint64 Settings::iniUInt64(const QString &key, quint64 defaultValue) const
{
    return iniValue(key, defaultValue).toULongLong();
}

qreal Settings::iniReal(const QString &key, qreal defaultValue) const
{
    return iniValue(key, defaultValue).toReal();
}

QString Settings::iniText(const QString &key, const QString &defaultValue) const
{
    return iniValue(key, defaultValue).toString();
}

QStringList Settings::iniList(const QString &key) const
{
    return iniValue(key).toStringList();
}

QVariantMap Settings::iniMap(const QString &key) const
{
    return iniValue(key).toMap();
}

QByteArray Settings::iniByteArray(const QString &key) const
{
    return iniValue(key).toByteArray();
}

QVariant Settings::iniValue(const QString &key, const QVariant &defaultValue) const
{
    if (key.isEmpty())
        return QVariant();

    const QString cachedKey = cacheKey(key);

    // Try to load from cache
    const QVariant cachedValue = cacheValue(cachedKey);
    if (!cachedValue.isNull())
        return cachedValue;

    // Load from .ini
    const auto value = m_ini->value(key, defaultValue);

    // Save to cache
    setCacheValue(cachedKey, value);

    return value;
}

void Settings::setIniValue(const QString &key, const QVariant &value, const QVariant &defaultValue)
{
    const QVariant oldValue = iniValue(key, defaultValue);
    if (oldValue == value)
        return;

    // Save to .ini
    saveIniValue(key, value);
}

QString Settings::cacheKey(const QString &key) const
{
    const QString group = m_ini->group();
    return group.isEmpty() ? key : group + '/' + key;
}

QVariant Settings::cacheValue(const QString &key) const
{
    return m_cache.value(key);
}

void Settings::setCacheValue(const QString &key, const QVariant &value) const
{
    m_cache.insert(key, value);
}

void Settings::reload()
{
    m_iniExists = true;

    clearCache();

    migrateIniOnLoad();
}

void Settings::clearCache()
{
    m_cache.clear();
    iniSync();
}

void Settings::saveIniValue(const QString &key, const QVariant &value)
{
    m_ini->setValue(key, value);

    // Save to cache
    setCacheValue(cacheKey(key), value);
}

void Settings::removeIniKey(const QString &key)
{
    m_ini->remove(key);
}

QStringList Settings::iniChildKeys(const QString &prefix) const
{
    m_ini->beginGroup(prefix);
    const QStringList list = m_ini->childKeys();
    m_ini->endGroup();
    return list;
}

void Settings::iniFlush()
{
    migrateIniOnWrite();

    iniSync();
    checkStatus();
}

void Settings::iniSync()
{
    m_ini->sync();
}

void Settings::checkStatus() const
{
    if (m_ini->status() != QSettings::NoError) {
        qCCritical(LC) << "Save error:" << m_ini->status() << "file:" << m_ini->fileName();
    }
}

int Settings::appVersion()
{
    return APP_VERSION;
}
