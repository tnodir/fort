#ifndef IOCCONTAINER_H
#define IOCCONTAINER_H

#include <QObject>
#include <QVarLengthArray>

class IocService;

using IocObject = void;

constexpr int IOC_MAX_SIZE = 32;

class IocContainer : public QObject
{
    Q_OBJECT

public:
    enum IocFlag : quint8 { AutoDelete = 0x01, IsService = 0x02, WasSetUp = 0x04 };

    explicit IocContainer(QObject *parent = nullptr);
    ~IocContainer() override;

    const int size() const { return m_size; }

    template<class T>
    void set(T &obj)
    {
        setObject(getTypeId<T>(), &obj);
    }

    template<class T>
    void set(T *obj)
    {
        setObject(getTypeId<T>(), obj, AutoDelete);
    }

    template<class T>
    void remove()
    {
        setObject(getTypeId<T>(), nullptr);
    }

    template<class T>
    void setService(T &obj)
    {
        Q_ASSERT(static_cast<IocService *>(&obj));
        setObject(getTypeId<T>(), &obj, IsService);
    }

    template<class T>
    void setService(T *obj)
    {
        Q_ASSERT(static_cast<IocService *>(obj));
        setObject(getTypeId<T>(), obj, AutoDelete | IsService);
    }

    template<class T>
    T *resolve() const
    {
        return static_cast<T *>(resolveObject(getTypeId<T>()));
    }

    void setUpAll();
    void tearDownAll();

    template<class T>
    void setUpDependency()
    {
        setUp(getTypeId<T>());
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
    QVarLengthArray<IocObject *, IOC_MAX_SIZE> m_objects;
    QVarLengthArray<quint8, IOC_MAX_SIZE> m_objectFlags;
};

template<class T>
T *IoC()
{
    const IocContainer *container = IocContainer::getPinned();
    Q_ASSERT(container);
    return container->resolve<T>();
}

#define IoC() IocContainer::getPinned()

#endif // IOCCONTAINER_H
