#include "variantutil.h"

QVariantList VariantUtil::vectorToList(const QVector<qint64> &array)
{
    QVariantList list;
    list.reserve(array.size());

    for (const qint64 v : array) {
        list.append(v);
    }

    return list;
}

QVector<qint64> VariantUtil::listToVector(const QVariantList &list)
{
    QVector<qint64> array;
    array.reserve(list.size());

    for (const QVariant &v : list) {
        array.append(v.toLongLong());
    }

    return array;
}

void VariantUtil::addToList(QVariantList &list, const QVariant &v)
{
    list.push_back(v); // append() merges the list, does not insert
}
