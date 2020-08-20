#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <QObject>
#include <QVariant>

class JsonUtil
{
public:
    static QVariant jsonToVariant(const QByteArray &data, QString &errorString);
};

#endif // JSONUTIL_H
