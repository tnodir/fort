#ifndef IOCOBJECT_H
#define IOCOBJECT_H

class IocObject
{
public:
    explicit IocObject() : m_autoDelete(true), m_wasSetUp(false) { }
    virtual ~IocObject() = default;

    virtual void setUp() { setWasSetUp(true); }
    virtual void tearDown() { }

    bool autoDelete() const { return m_autoDelete; }
    void setAutoDelete(bool v) { m_autoDelete = v; }

    bool wasSetUp() const { return m_wasSetUp; }
    void setWasSetUp(bool v) { m_wasSetUp = v; }

private:
    bool m_autoDelete : 1;
    bool m_wasSetUp : 1;
};

#endif // IOCOBJECT_H
