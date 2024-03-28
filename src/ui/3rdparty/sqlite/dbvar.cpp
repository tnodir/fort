#include "dbvar.h"

#include "sqlitedb.h"
#include "sqlitestmt.h"

QVariant DbVar::nullable(int v)
{
    return nullable(v, v == 0);
}

QVariant DbVar::nullable(const QString &v)
{
    return nullable(v, v.isEmpty());
}

QVariant DbVar::nullable(const QDateTime &v)
{
    return nullable(v, v.isNull());
}
