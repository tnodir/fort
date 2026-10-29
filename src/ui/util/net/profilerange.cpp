#include "profilerange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>

namespace {

struct ProfileType
{
    quint8 id;
    const char *name; // also the translated text's source
};

// By their ids: Public, Private, Domain
inline constexpr ProfileType g_profileTypes[] = {
    { FORT_RULE_FILTER_PROFILE_PUBLIC, QT_TRANSLATE_NOOP("ProfileRange", "Public") },
    { FORT_RULE_FILTER_PROFILE_PRIVATE, QT_TRANSLATE_NOOP("ProfileRange", "Private") },
    { FORT_RULE_FILTER_PROFILE_DOMAIN, QT_TRANSLATE_NOOP("ProfileRange", "Domain") },
};

}

ProfileRange::ProfileRange(QObject *parent) : TextRange(parent) { }

bool ProfileRange::isEmpty() const
{
    return m_profileId == 0;
}

void ProfileRange::clear()
{
    TextRange::clear();

    m_profileId = 0;
}

void ProfileRange::toList(QStringList &list) const
{
    for (const auto &profileType : g_profileTypes) {
        if (profileType.id == m_profileId) {
            list << QString(profileType.name).toUpper();
        }
    }
}

quint8 ProfileRange::profileIdByName(const QString &name)
{
    for (const auto &profileType : g_profileTypes) {
        if (name.compare(QLatin1String(profileType.name), Qt::CaseInsensitive) == 0) {
            return profileType.id;
        }
    }

    return 0;
}

QStringList ProfileRange::profileTexts()
{
    QStringList list;
    for (const auto &profileType : g_profileTypes) {
        list << tr(profileType.name);
    }
    return list;
}

TextRange::ParseError ProfileRange::parseText(const QString &text)
{
    const quint8 profileId = profileIdByName(text);
    if (profileId == 0)
        return ErrorBadText;

    m_profileId = profileId;

    return ErrorOk;
}

void ProfileRange::write(ConfData &confData) const
{
    confData.writeProfileRange(*this);
}
