#include "hostrange.h"

#include <QUrl>

#include <algorithm>

#include <common/fortconf.h>

#include <util/conf/confdata.h>
#include <util/stringutil.h>

namespace {

inline constexpr int nameLengthMax = 253;
inline constexpr int labelLengthMax = 63;

inline constexpr char wildcardPrefix[] = "*.";
inline constexpr char domainPrefix = '.'; // the name and its subdomains

bool isAsciiChar(QChar c)
{
    return c.unicode() < 0x80;
}

// An internationalized name's labels in Punycode; empty for a bad one
QString asciiName(const QString &name)
{
    if (std::all_of(name.begin(), name.end(), isAsciiChar))
        return name;

    return QString::fromLatin1(QUrl::toAce(name));
}

bool isLabelCharValid(QChar c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

bool isPatternCharValid(QChar c)
{
    return c == '*' || isLabelCharValid(c);
}

bool isLabelValid(QStringView label, bool isPattern)
{
    const int size = label.size();
    if (size == 0 || size > labelLengthMax)
        return false;

    if (label.startsWith('-') || label.endsWith('-'))
        return false;

    return std::all_of(
            label.begin(), label.end(), isPattern ? &isPatternCharValid : &isLabelCharValid);
}

bool isNameValid(const QString &name, bool isPattern = false)
{
    if (name.isEmpty() || name.size() > nameLengthMax)
        return false;

    const auto labels = StringUtil::splitView(name, QLatin1Char('.'));

    return std::all_of(labels.begin(), labels.end(),
            [=](QStringView label) { return isLabelValid(label, isPattern); });
}

// A name with a mask other than the "*." prefix
bool isPatternName(const QString &name)
{
    const int from = name.startsWith(QLatin1String(wildcardPrefix)) ? 1 : 0;

    return name.indexOf(QLatin1Char('*'), from) >= 0;
}

bool isPatternValid(const QString &pattern)
{
    if (pattern.contains(QLatin1String("**")))
        return false;

    // Not of the masks only
    if (!std::any_of(pattern.begin(), pattern.end(), isLabelCharValid))
        return false;

    return isNameValid(pattern, /*isPattern=*/true);
}

}

HostRange::HostRange(QObject *parent) : ValueRange(parent) { }

bool HostRange::isEmpty() const
{
    return size() == 0;
}

int HostRange::sizeToWrite() const
{
    return sizeof(FORT_CONF_RULE_FILTER_HOST); // the id: the driver doesn't get the names
}

void HostRange::clear()
{
    ValueRange::clear();

    m_names.clear();
    m_wildcardNames.clear();
    m_patternNames.clear();
}

void HostRange::toList(QStringList &list) const
{
    QStringList names = m_names.values() + m_patternNames.values();

    for (const auto &name : m_wildcardNames) {
        names << (wildcardPrefix + name);
    }

    names.sort();

    list << names;
}

bool HostRange::fromList(const StringViewList &list, bool /*sort*/)
{
    clear();

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        if (parseName(lineTrimmed) != ErrorOk) {
            appendErrorDetails(QString("line='%1'").arg(line));
            setErrorLineNo(lineNo);
            return false;
        }
    }

    return true;
}

void HostRange::write(ConfData &confData) const
{
    confData.writeHostRange(*this);
}

QString HostRange::normalizedName(QStringView name)
{
    QString s = name.toString().toLower();
    if (s.endsWith('.')) {
        s.chop(1);
    }
    return s;
}

QByteArray HostRange::normalizedKey(QStringView name)
{
    return normalizedName(name).toLatin1();
}

HostRange::ParseError HostRange::parseName(QStringView line)
{
    const QString name = normalizedName(line);

    return isPatternName(name) ? parsePattern(name) : parsePlainName(name);
}

HostRange::ParseError HostRange::parsePlainName(QString name)
{
    const bool isDomain = name.startsWith(QLatin1Char(domainPrefix));
    const bool isWildcard = isDomain || name.startsWith(QLatin1String(wildcardPrefix));
    if (isWildcard) {
        name.remove(0, isDomain ? 1 : int(sizeof(wildcardPrefix) - 1));
    }

    name = asciiName(name);

    if (!isNameValid(name)) {
        setErrorMessage(tr("Bad host name"));
        return ErrorBadName;
    }

    if (isWildcard) {
        m_wildcardNames.insert(name);
    }
    if (isDomain || !isWildcard) {
        m_names.insert(name);
    }

    return ErrorOk;
}

HostRange::ParseError HostRange::parsePattern(const QString &pattern)
{
    if (!isPatternValid(pattern)) {
        setErrorMessage(tr("Bad host name"));
        return ErrorBadName;
    }

    m_patternNames.insert(pattern);

    if (m_patternNames.size() > patternNamesMax) {
        setErrorMessage(tr("Too many host name masks"));
        return ErrorTooManyPatterns;
    }

    return ErrorOk;
}
