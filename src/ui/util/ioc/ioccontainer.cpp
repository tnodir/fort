#include "ioccontainer.h"

#include <QLoggingCategory>

#include "iocservice.h"

namespace {
const QLoggingCategory LC("util.ioc.iocContainer");
}

ThreadStorage IocContainer::g_threadStorage;

IocContainer::IocContainer() = default;

IocContainer::~IocContainer()
{
    autoDeleteAll();
}

void IocContainer::setObject(int typeId, IocObject *obj, quint8 flags)
{
    const int newSize = typeId + 1;
    if (newSize > m_size) {
        if (Q_UNLIKELY(newSize >= IOC_MAX_SIZE)) {
            qCCritical(LC) << "IoC Container size error";
            Q_UNREACHABLE();
        }
        m_size = newSize;
    }

    m_objects[typeId] = obj;

    Q_ASSERT(obj || flags == 0);
    m_objectFlags[typeId] = flags;
}

void IocContainer::setUpAll()
{
    for (int i = 0; i < m_size; ++i) {
        setUp(i);
    }
}

void IocContainer::tearDownAll()
{
    int i = m_setupIndex;
    while (--i >= 0) {
        tearDown(m_objectSetupIds[i]);
    }
}

void IocContainer::autoDeleteAll()
{
    for (int i = 0; i < m_size; ++i) {
        autoDelete(i);
    }
}

IocService *IocContainer::setUp(int typeId)
{
    IocService *obj = resolveService(typeId);

    const quint8 flags = m_objectFlags[typeId];
    if ((flags & (IsService | WasSetUp)) == IsService) {
        m_objectFlags[typeId] = (flags | WasSetUp);
        m_objectSetupIds[m_setupIndex++] = typeId;

        obj->setUp();
    }

    return obj;
}

void IocContainer::tearDown(int typeId)
{
    const quint8 flags = m_objectFlags[typeId];
    if (!(flags & IsService))
        return;

    IocService *obj = resolveService(typeId);
    obj->tearDown();
}

void IocContainer::autoDelete(int typeId)
{
    const quint8 flags = m_objectFlags[typeId];
    if ((flags & (AutoDelete | IsService)) != (AutoDelete | IsService))
        return;

    IocService *obj = resolveService(typeId);
    setObject(typeId, nullptr);

    delete obj;
}

bool IocContainer::pinToThread()
{
    return g_threadStorage.setValue(this);
}

int IocContainer::getNextTypeId()
{
    static int g_nextTypeId = 0;
    return g_nextTypeId++;
}
