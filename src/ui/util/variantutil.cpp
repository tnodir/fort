#include "variantutil.h"

namespace {

const char *const UserDataKey = "userData";

}

QVariant VariantUtil::userData(QObject *o)
{
    return o->property(UserDataKey);
}

void VariantUtil::setUserData(QObject *o, const QVariant &v)
{
    o->setProperty(UserDataKey, v);
}
