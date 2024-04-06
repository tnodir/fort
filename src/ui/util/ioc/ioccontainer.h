#ifndef IOCCONTAINER_H
#define IOCCONTAINER_H

#include <QObject>

#include <util/threadstorage.h>

#include "iocservice.h"

using IocObject = void;

constexpr int IOC_MAX_SIZE = 32;

class IocContainer final
{
public:
    enum IocFlag : quint8 { AutoDelete = 0x01, IsService = 0x02, WasSetUp = 0x04 };

    explicit IocContainer();
    ~IocContainer();

    const int size() const { return m_size; }

    template<class T>
    void set(T *obj)
    {
        setObject(getTypeId<T>(), obj);
    }

    template<class T>
    void set(T &obj)
    {
        set(&obj);
    }

    template<class T>
    void remove()
    {
        set<T>(nullptr);
    }

    template<class T>
    void setService(T *obj, quint8 flags = AutoDelete | IsService)
    {
        auto svc = static_cast<IocService *>(obj);
        Q_ASSERT(svc);
        setObject(getTypeId<T>(), svc, flags);
    }

    template<class T>
    void setService(T &obj)
    {
        setService(&obj, IsService);
    }

    template<class T>
    inline constexpr std::enable_if_t<!std::is_base_of_v<IocService, T>, T *> resolve() const
    {
        return static_cast<T *>(resolveObject(getTypeId<T>()));
    }

    template<class T>
    inline constexpr std::enable_if_t<std::is_base_of_v<IocService, T>, T *> resolve() const
    {
        return static_cast<T *>(resolveService(getTypeId<T>()));
    }

    template<class T>
    inline constexpr T *setUpDependency()
    {
        return static_cast<T *>(setUp(getTypeId<T>()));
    }

    void setUpAll();
    void tearDownAll();
    void autoDeleteAll();

    bool pinToThread();

    inline static IocContainer *getPinned()
    {
        return static_cast<IocContainer *>(g_threadStorage.value());
    }

    template<class T>
    static int getTypeId()
    {
        static const int typeId = getNextTypeId();
        Q_ASSERT(typeId < IOC_MAX_SIZE);
        return typeId;
    }

private:
    void setObject(int typeId, IocObject *obj, quint8 flags = 0);

    inline IocObject *resolveObject(int typeId) const { return m_objects[typeId]; }
    inline IocService *resolveService(int typeId) const
    {
        return static_cast<IocService *>(resolveObject(typeId));
    }

    IocService *setUp(int typeId);
    void tearDown(int typeId);
    void autoDelete(int typeId);

    static int getNextTypeId();

private:
    static ThreadStorage g_threadStorage;

    int m_size = 0;
    int m_setupIndex = 0;

    quint8 m_objectFlags[IOC_MAX_SIZE] = {};
    quint16 m_objectSetupIds[IOC_MAX_SIZE] = {};
    IocObject *m_objects[IOC_MAX_SIZE] = {};
};

constexpr auto IoCPinned = IocContainer::getPinned;

template<class T>
inline static T *IoC()
{
    return IoCPinned()->resolve<T>();
}

template<class T>
inline static T *IoCDependency()
{
    return IoCPinned()->setUpDependency<T>();
}

#endif // IOCCONTAINER_H
