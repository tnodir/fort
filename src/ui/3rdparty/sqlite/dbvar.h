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
    inline static QVariant nullable(bool v) { return nullable(v, !v); }
    inline static QVariant nullable(int v) { return nullable(v, v == 0); }
    inline static QVariant nullable(quint32 v) { return nullable(v, v == 0); }
    inline static QVariant nullable(qint64 v) { return nullable(v, v == 0); }
    static QVariant nullable(const QString &v);
    static QVariant nullable(const QDateTime &v);
};

#endif // DBVAR_H
