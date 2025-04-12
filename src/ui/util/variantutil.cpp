#include "variantutil.h"

namespace {

const char *const UserDataKey = "userData";

}

QStringList VariantUtil::listToStringList(const QVariantList &list, Qt::SplitBehavior behavior)
{
    QStringList res;

    for (const QVariant &v : list) {
        const auto s = v.toString();
        if (s.isEmpty() && behavior == Qt::SkipEmptyParts)
            continue;

        res.append(s);
    }

    return res;
}

QVariant VariantUtil::userData(QObject *o)
{
    return o->property(UserDataKey);
}

void VariantUtil::setUserData(QObject *o, const QVariant &v)
{
    o->setProperty(UserDataKey, v);
}
