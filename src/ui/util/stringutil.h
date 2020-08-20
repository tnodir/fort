#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QObject>

#include "../fortcompat.h"

class StringUtil
{
public:
    static QString capitalize(const QString &text);

    static QString cryptoHash(const QString &text);

    static int lineStart(const QString &text, int pos,
                         int badPos = -1);
    static int lineEnd(const QString &text, int pos,
                       int badPos = -1);

    static StringViewList splitView(const QString &text, QLatin1Char sep,
                                    bool skipEmptyParts = false);
};

#endif // STRINGUTIL_H
