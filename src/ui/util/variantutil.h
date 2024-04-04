#ifndef VARIANTUTIL_H
#define VARIANTUTIL_H

#include <QObject>
#include <QVariant>
#include <QVector>

class VariantUtil
{
public:
    template<typename T>
    static void vectorToList(const QVector<T> &array, QVariantList &list);

    template<typename T>
    static void listToVector(const QVariantList &list, QVector<T> &array);

    template<typename T>
    inline static void addToList(QList<T> &list, const T &v)
    {
        list.push_back(v); // append() merges the list, does not insert
    }

    static QVariant userData(QObject *o);
    static void setUserData(QObject *o, const QVariant &v = {});
};

template<typename T>
void VariantUtil::vectorToList(const QVector<T> &array, QVariantList &list)
{
    list.reserve(array.size());

    for (const T v : array) {
        list.append(v);
    }
}

template<typename T>
void VariantUtil::listToVector(const QVariantList &list, QVector<T> &array)
{
    array.reserve(list.size());

    for (const QVariant &v : list) {
        array.append(v.value<T>());
    }
}

#endif // VARIANTUTIL_H
