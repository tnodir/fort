#ifndef VARIANTUTIL_H
#define VARIANTUTIL_H

#include <QObject>
#include <QVariant>
#include <QVector>

class VariantUtil
{
public:
    static QVariantList vectorToList(const QVector<qint64> &array);

    static QVector<qint64> listToVector(const QVariantList &list);

    static void addToList(QVariantList &list, const QVariant &v);
};

#endif // VARIANTUTIL_H
