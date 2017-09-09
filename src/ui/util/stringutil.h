#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QObject>

class StringUtil : public QObject
{
    Q_OBJECT

public:
    explicit StringUtil(QObject *parent = nullptr);

    static QString capitalize(const QString &text);
};

#endif // STRINGUTIL_H
