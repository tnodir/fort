#ifndef CLASSUTIL_H
#define CLASSUTIL_H

#include <QMetaMethod>
#include <QVariant>

class ClassUtil
{
public:
    static bool invokeMethod(QObject *object, int methodIndex, const QVariantList &args = {},
            QVariant *result = nullptr);

    template<typename PointerToMemberFunction>
    static int indexOfSignal(PointerToMemberFunction signal)
    {
        const QMetaMethod method = QMetaMethod::fromSignal(signal);
        return method.methodIndex();
    }

    template<typename PointerToMemberFunction>
    static int indexOfMethod(PointerToMemberFunction method)
    {
        using MemberFunctionType = QtPrivate::FunctionPointer<PointerToMemberFunction>;
        return getIndexOfMethod(
                MemberFunctionType::Object::staticMetaObject, reinterpret_cast<void **>(&method));
    }

private:
    static int getIndexOfMethod(const QMetaObject &metaObject, void **method);
};

#endif // CLASSUTIL_H
