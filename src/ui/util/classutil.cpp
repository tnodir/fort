#include "classutil.h"

bool ClassUtil::invokeMethod(
        QObject *object, int methodIndex, const QVariantList &args, QVariant *result)
{
    const QMetaMethod metaMethod = object->metaObject()->method(methodIndex);

    constexpr int maxArgsCount = 5;
    Q_ASSERT(metaMethod.parameterTypes().size() == args.size() && args.size() <= maxArgsCount);

    QVector<QGenericArgument> arguments(maxArgsCount);

    for (const auto &arg : args) {
        void *data = const_cast<void *>(arg.constData()); // data() detaches it
        arguments.append(QGenericArgument(arg.typeName(), data));
    }

    QVariant returnValue(metaMethod.returnMetaType(), nullptr);
    QGenericReturnArgument returnArgument(
            metaMethod.typeName(), const_cast<void *>(returnValue.constData()));

    if (!metaMethod.invoke(object, Qt::DirectConnection, returnArgument, arguments.value(0),
                arguments.value(1), arguments.value(2), arguments.value(3), arguments.value(4)))
        return false;

    if (result) {
        *result = returnValue;
    }
    return true;
}

int ClassUtil::getIndexOfMethod(const QMetaObject &metaObject, void **method)
{
    int i = -1;
    void *args[] = { &i, method };
    metaObject.static_metacall(QMetaObject::IndexOfMethod, 0, args);
    return i;
}
