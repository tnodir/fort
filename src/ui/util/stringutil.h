#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QObject>
#include <QRegularExpression>

#include "util_types.h"

class StringUtil
{
public:
    static QString capitalize(const QString &text);

    static QString cryptoHash(const QString &text);

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

    static QRegularExpressionMatch match(const QRegularExpression &re, const QStringView &text);
};

#endif // STRINGUTIL_H
