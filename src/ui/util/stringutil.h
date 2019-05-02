#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QObject>

class StringUtil : public QObject
{
    Q_OBJECT

public:
    explicit StringUtil(QObject *parent = nullptr);

    Q_INVOKABLE static QString capitalize(const QString &text);

    Q_INVOKABLE static QString cryptoHash(const QString &text);

    Q_INVOKABLE static int lineStart(const QString &text, int pos,
                                     int badPos = -1);
    Q_INVOKABLE static int lineEnd(const QString &text, int pos,
                                   int badPos = -1);
};

#endif // STRINGUTIL_H
