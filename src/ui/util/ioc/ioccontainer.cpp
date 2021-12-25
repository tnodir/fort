#include "ioccontainer.h"

#include <QLoggingCategory>

#include "iocservice.h"

namespace {
const QLoggingCategory LC("util.ioc.iocContainer");
}

int IocContainer::g_tlsIndex = -1;

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
            abort();
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
    for (int i = 0; i < m_size; ++i) {
        tearDown(i);
    }
}

void IocContainer::autoDeleteAll()
{
    for (int i = 0; i < m_size; ++i) {
        autoDelete(i);
    }
}

void IocContainer::setUp(int typeId)
{
    const quint8 flags = m_objectFlags[typeId];
    if ((flags & (IsService | WasSetUp)) != IsService)
        return;

    m_objectFlags[typeId] = (flags | WasSetUp);

    IocService *obj = resolveService(typeId);
    obj->setUp();
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
    createTlsIndex();

    return TlsSetValue(g_tlsIndex, this);
}

void IocContainer::createTlsIndex()
{
    if (g_tlsIndex == -1) {
        g_tlsIndex = TlsAlloc();
        if (g_tlsIndex == -1) {
            qCCritical(LC) << "TlsAlloc error";
        }
    }
}

void IocContainer::deleteTlsIndex()
{
    if (g_tlsIndex != -1) {
        TlsFree(g_tlsIndex);
        g_tlsIndex = -1;
    }
}

int IocContainer::getNextTypeId()
{
    static int g_nextTypeId = 0;
    return g_nextTypeId++;
}
