#ifndef CLASSUTIL_H
#define CLASSUTIL_H

#include <QMetaMethod>
#include <QVariant>

class ClassUtil
{
public:
    static bool invokeMethod(QObject *o, const QMetaMethod method, const QVariantList &args);

    template<typename Func>
    static int indexOfSignal(Func signal)
    {
        const QMetaMethod method = QMetaMethod::fromSignal(signal);
        return method.methodIndex();
    }

    template<typename Func>
    static int indexOfMethod(Func method)
    {
        using FuncType = QtPrivate::FunctionPointer<Func>;
        return getIndexOfMethod(
                FuncType::Object::staticMetaObject, reinterpret_cast<void **>(&method));
    }

private:
    static int getIndexOfMethod(const QMetaObject &metaObj, void **method);
};

#endif // CLASSUTIL_H
