#include "stringutil.h"

#include <QCryptographicHash>

QString StringUtil::capitalize(const QString &text)
{
    const QChar firstChar = text.at(0);

    return firstChar.toUpper() + text.mid(1);
}

QString StringUtil::cryptoHash(const QString &text)
{
    if (text.isEmpty())
        return QString();

    const QByteArray data = text.toUtf8();
    const QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);

    return QString::fromLatin1(hash.toHex());
}

int StringUtil::lineStart(const QString &text, int pos, int badPos)
{
    const int startPos = text.lastIndexOf(QLatin1Char('\n'), pos);
    return (startPos != -1) ? startPos : badPos;
}

int StringUtil::lineEnd(const QString &text, int pos, int badPos)
{
    const int endPos = text.indexOf(QLatin1Char('\n'), pos);
    return (endPos != -1) ? endPos : badPos;
}

StringViewList StringUtil::splitView(const QString &text, QLatin1Char sep, bool skipEmptyParts)
{
    const auto behavior = skipEmptyParts ? Qt::SkipEmptyParts : Qt::KeepEmptyParts;
    return QStringView(text).split(sep, behavior);
}

TokenizeViewResult StringUtil::tokenizeView(
        const QString &text, QLatin1Char sep, bool skipEmptyParts)
{
    const auto behavior = skipEmptyParts ? Qt::SkipEmptyParts : Qt::KeepEmptyParts;
    return QStringView(text).tokenize(sep, behavior);
}

void StringUtil::addStringToBuffer(QByteArray &buffer, const QString &s)
{
    const int bufferSize = buffer.size();
    buffer.resize(bufferSize + (s.size() + 1) * sizeof(wchar_t)); // + terminating null character

    wchar_t *cp = (wchar_t *) (buffer.data() + bufferSize);
    s.toWCharArray(cp);
    cp[s.size()] = L'\0';
}

bool StringUtil::buildMultiString(QByteArray &buffer, const QStringList &list)
{
    for (const QString &s : list) {
        if (s.isEmpty())
            return false; // Multi-String value cannot contain an empty string

        addStringToBuffer(buffer, s);
    }

    addStringToBuffer(buffer, {});

    return true;
}

QStringList StringUtil::parseMultiString(const char *data)
{
    QStringList list;
    const wchar_t *cp = (const wchar_t *) data;
    for (;;) {
        const QString s = QString::fromWCharArray(cp);
        if (s.isEmpty())
            break; // Multi-String value cannot contain an empty string

        list.append(s);

        cp += s.size() + 1; // + terminating null character
    }
    return list;
}

QString StringUtil::firstLine(const QString &text)
{
    const int pos = text.indexOf('\n');
    return (pos != -1) ? text.left(pos) : text;
}

QRegularExpressionMatch StringUtil::match(const QRegularExpression &re, const QStringView &text)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    return re.matchView(text);
#else
    return re.match(text);
#endif
}
