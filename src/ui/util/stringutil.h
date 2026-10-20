#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QObject>
#include <QRegularExpression>

#include "util_types.h"

class StringUtil
{
public:
    static QString capitalize(const QString &text);

    static QByteArray cryptoSha1(const QByteArray &data);
    static QString cryptoHash(const QString &text);

    static QByteArray serviceSid(const QString &serviceName);

    static int lineStart(const QString &text, int pos, int badPos = -1);
    static int lineEnd(const QString &text, int pos, int badPos = -1);

    static StringViewList splitView(
            const QString &text, QLatin1Char sep, bool skipEmptyParts = false);
    static TokenizeViewResult tokenizeView(
            const QString &text, QLatin1Char sep, bool skipEmptyParts = false);

    static void addStringToBuffer(QByteArray &buffer, const QString &s);
    static bool buildMultiString(QByteArray &buffer, const QStringList &list);
    static QStringList parseMultiString(const char *data);

    static QString firstLine(const QString &text);

    // The text with its details in parentheses, without them when empty: e.g. "Groups (2)"
    static QString textWithDetails(const QString &text, const QString &details);
    static QString textWithCount(const QString &text, int count);

    static QRegularExpressionMatch match(const QRegularExpression &re, const QStringView text);
};

#endif // STRINGUTIL_H
