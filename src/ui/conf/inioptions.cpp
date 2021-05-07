#include "inioptions.h"

#include "../fortsettings.h"

IniOptions::IniOptions(const IniOptions &o) : MapWrapper(o.map()) { }

QString IniOptions::defaultLanguage() const
{
    return settings() ? settings()->defaultLanguage() : "en";
}

QVariant IniOptions::value(const QString &key, const QVariant &defaultValue) const
{
    const QVariant v = MapWrapper::value(key);
    if (!v.isNull())
        return v;

    return settings() ? settings()->iniValue(key, defaultValue) : defaultValue;
}

bool IniOptions::isTransientKey(const QString &key)
{
    return key.endsWith('_');
}

void IniOptions::save(FortSettings *settings) const
{
    auto it = map().constBegin();
    const auto end = map().constEnd();

    for (; it != end; ++it) {
        const QString &key = it.key();
        if (!isTransientKey(key)) {
            settings->setIniValue(key, it.value());
        }
    }

    if (hasPassword() != settings->hasPassword() || !password().isEmpty()) {
        settings->setPassword(password());
        if (!hasPassword()) {
            settings->resetCheckedPassword();
        }
    }
}
