#ifndef METACLASSUTIL_H
#define METACLASSUTIL_H

#include <QMetaMethod>

class MetaClassUtil
{
public:
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

#endif // METACLASSUTIL_H
