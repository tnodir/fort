#include "envmanager.h"

#include <QRegularExpression>
#include <QSettings>

#include <util/fileutil.h>

EnvManager::EnvManager(QObject *parent) : QObject(parent)
{
    setupDefaultKeys();
}

QString EnvManager::expandString(const QString &text)
{
    return expandStringRecursive(text);
}

void EnvManager::clearCache()
{
    m_cache.clear();

    setupDefaultKeys();
}

void EnvManager::onEnvironmentChanged()
{
    if (checkEnvironmentChanged()) {
        clearCache();

        emit environmentUpdated();
    }
}

bool EnvManager::checkEnvironmentChanged() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    for (const auto &[key, value] : std::as_const(m_cache).asKeyValueRange()) {
#else
    auto it = m_cache.constBegin();
    for (; it != m_cache.constEnd(); ++it) {
        const auto &key = it.key();
        const auto &value = it.value();
#endif

        const auto newValue = readEnvVar(key);

        if (value != newValue)
            return true;
    }

    return false;
}

void EnvManager::setupDefaultKeys()
{
    setCachedEnvVar(envFortHome(), FileUtil::appBinLocation());
}

QString EnvManager::expandStringRecursive(const QString &text, quint16 callLevel)
{
    static const QRegularExpression re("%([^%]+)%");

    if (!text.contains(QLatin1Char('%')))
        return text;

    constexpr int maxCallLevel = 9;
    if (callLevel >= maxCallLevel)
        return QString(); // avoid infinite cycling

    QString res = text;

    auto i = re.globalMatch(text);

    while (i.hasNext()) {
        const auto match = i.next();
        const auto subKey = match.captured(1);

        QString value = envVar(subKey);
        value = expandStringRecursive(value, callLevel + 1);

        res.replace('%' + subKey + '%', value);
    }

    res.replace("%%", "%"); // normalize escaped symbol

    return res;
}

QString EnvManager::envVar(const QString &key)
{
    auto value = m_cache.value(key);
    if (value.isNull()) {
        value = readEnvVar(key);
        setCachedEnvVar(key, value);
    }
    return value.toString();
}

void EnvManager::setCachedEnvVar(const QString &key, const QVariant &value)
{
    m_cache.insert(key, value);
}

QVariant EnvManager::readEnvVar(const QString &key)
{
    const auto userVar = readRegVar(key, "HKEY_CURRENT_USER\\Environment");
    if (!userVar.isNull())
        return userVar;

    const auto sysVar = readRegVar(key,
            "HKEY_LOCAL_MACHINE\\SYSTEM"
            "\\CurrentControlSet\\Control"
            "\\Session Manager\\Environment");
    if (!sysVar.isNull())
        return sysVar;

    return qEnvironmentVariable(key.toLatin1());
}

QVariant EnvManager::readRegVar(const QString &key, const char *envPath)
{
    const QSettings reg(envPath, QSettings::NativeFormat);
    return reg.value(key);
}
