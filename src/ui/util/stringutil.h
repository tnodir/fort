#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QObject>

class StringUtil
{
public:
    static QString capitalize(const QString &text);

    static QString cryptoHash(const QString &text);

    static int lineStart(const QString &text, int pos,
                         int badPos = -1);
    static int lineEnd(const QString &text, int pos,
                       int badPos = -1);
};

#endif // STRINGUTIL_H
