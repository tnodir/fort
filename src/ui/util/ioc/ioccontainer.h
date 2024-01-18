#ifndef IOCCONTAINER_H
#define IOCCONTAINER_H

#include <QObject>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

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

    void setUpAll();
    void tearDownAll();
    void autoDeleteAll();

    template<class T>
    inline constexpr T *setUpDependency()
    {
        setUp(getTypeId<T>());
        return resolve<T>();
    }

    bool pinToThread();

    inline static IocContainer *getPinned()
    {
        return static_cast<IocContainer *>(TlsGetValue(g_tlsIndex));
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

    void setUp(int typeId);
    void tearDown(int typeId);
    void autoDelete(int typeId);

    static void createTlsIndex();
    static void deleteTlsIndex();

    static int getNextTypeId();

private:
    static int g_tlsIndex;

    int m_size = 0;

    quint8 m_objectFlags[IOC_MAX_SIZE] = {};
    IocObject *m_objects[IOC_MAX_SIZE] = {};
};

constexpr auto IoCPinned = IocContainer::getPinned;

template<class T>
inline static T *IoC()
{
    const IocContainer *container = IoCPinned();
    Q_ASSERT(container);
    return container->resolve<T>();
}

#endif // IOCCONTAINER_H
