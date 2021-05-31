#ifndef IOCCONTAINER_H
#define IOCCONTAINER_H

#include <QObject>
#include <QVector>

class IocObject
{
public:
    virtual void setUp() { setWasSetUp(true); }
    virtual void tearDown() { }

    bool wasSetUp() const { return m_wasSetUp; }
    void setWasSetUp(bool v) { m_wasSetUp = v; }

private:
    bool m_wasSetUp = false;
};

class IocContainer : public QObject
{
    Q_OBJECT

public:
    explicit IocContainer(QObject *parent = nullptr);
    ~IocContainer() override;

    const QVector<IocObject *> &objects() const { return m_objects; }

    template<class T>
    void put(T *obj)
    {
        QObject *qObj = qobject_cast<QObject *>(obj);
        Q_ASSERT(qObj);
        qObj->setParent(this);

        insert(obj);
    }

    template<class T>
    void insert(T *obj)
    {
        insertObject(getTypeId<T>(), obj);
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
        setUp(resolve<T>());
    }

    void setUp(IocObject *obj);

    bool pinToThread();

    static IocContainer *getPinned();

    template<class T>
    static int getTypeId()
    {
        static int typeId = getNextTypeId();
        return typeId;
    }

private:
    void insertObject(int typeId, IocObject *obj);
    IocObject *resolveObject(int typeId) const;

    static int getNextTypeId();

private:
    QVector<IocObject *> m_objects;
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
