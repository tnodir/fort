#include "classutil.h"

bool ClassUtil::invokeMethod(QObject *o, const QMetaMethod method, const QVariantList &args)
{
    // TODO: method.invoke(o, Q_ARG(T, args.at(0).to<T>()) ...);
    return false;
}

int ClassUtil::getIndexOfMethod(const QMetaObject &metaObj, void **method)
{
    int methodIndex = -1;
    void *metaArgs[] = { &methodIndex, reinterpret_cast<void **>(&method) };
    metaObj.static_metacall(QMetaObject::IndexOfMethod, 0, metaArgs);
    return methodIndex;
}
