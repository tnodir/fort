#ifndef DBVAR_H
#define DBVAR_H

#include <QObject>
#include <QVariant>

class DbVar
{
public:
    template<class T>
    inline static QVariant nullable(const T &v, bool isNull)
    {
        return isNull ? QVariant() : QVariant(v);
    }
    static QVariant nullable(int v);
    static QVariant nullable(const QString &v);
    static QVariant nullable(const QDateTime &v);
};

#endif // DBVAR_H
