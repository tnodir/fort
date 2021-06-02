#ifndef IOCCONTAINER_H
#define IOCCONTAINER_H

#include <QObject>
#include <QVarLengthArray>

#include "iocservice.h"

using IocObject = void;

constexpr int IOC_DEFAULT_SIZE = 32;

class IocContainer : public QObject
{
    Q_OBJECT

public:
    enum IocFlag : quint8 { AutoDelete = 0x01, IsService = 0x02, WasSetUp = 0x04 };

    explicit IocContainer(QObject *parent = nullptr);
    ~IocContainer() override;

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
    T *setUpDependency()
    {
        setUp(getTypeId<T>());
        return resolve<T>();
    }

    bool pinToThread();

    static IocContainer *getPinned();

    template<class T>
    static int getTypeId()
    {
        static const int typeId = getNextTypeId();
        return typeId;
    }

private:
    void setObject(int typeId, IocObject *obj, quint8 flags = 0);

    IocObject *resolveObject(int typeId) const;
    IocService *resolveService(int typeId) const;

    void setUp(int typeId);
    void tearDown(int typeId);
    void autoDelete(int typeId);

    static int getNextTypeId();

private:
    int m_size = 0;
    QVarLengthArray<IocObject *, IOC_DEFAULT_SIZE> m_objects;
    QVarLengthArray<quint8, IOC_DEFAULT_SIZE> m_objectFlags;
};

template<class T>
inline static T *IoC()
{
    const IocContainer *container = IocContainer::getPinned();
    Q_ASSERT(container);
    return container->resolve<T>();
}

#define IoC() IocContainer::getPinned()

#endif // IOCCONTAINER_H
