#include "metaclassutil.h"

int MetaClassUtil::getIndexOfMethod(const QMetaObject &metaObj, void **method)
{
    int methodIndex = -1;
    void *metaArgs[] = { &methodIndex, reinterpret_cast<void **>(&method) };
    metaObj.static_metacall(QMetaObject::IndexOfMethod, 0, metaArgs);
    return methodIndex;
}
