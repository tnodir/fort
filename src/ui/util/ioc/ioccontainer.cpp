#include "ioccontainer.h"

#include <QDebug>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "iocservice.h"

namespace {

int g_tlsIndex = -1;
int g_nextTypeId = 0;

}

IocContainer::IocContainer(QObject *parent) : QObject(parent) { }

IocContainer::~IocContainer()
{
    if (g_tlsIndex != -1) {
        TlsFree(g_tlsIndex);
        g_tlsIndex = -1;
    }
}

void IocContainer::setObject(int typeId, IocObject *obj, quint8 flags)
{
    const int newSize = typeId + 1;
    if (newSize > m_size) {
        m_size = newSize;
        m_objects.resize(newSize);
        m_objectFlags.resize(newSize);
    }

    m_objects[typeId] = obj;

    Q_ASSERT(obj || flags == 0);
    m_objectFlags[typeId] = flags;
}

IocObject *IocContainer::resolveObject(int typeId) const
{
    return m_objects.at(typeId);
}

IocService *IocContainer::resolveService(int typeId) const
{
    return static_cast<IocService *>(resolveObject(typeId));
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

    delete resolveService(typeId);

    setObject(typeId, nullptr);
}

bool IocContainer::pinToThread()
{
    if (g_tlsIndex == -1) {
        g_tlsIndex = TlsAlloc();
        if (g_tlsIndex == -1) {
            qWarning() << "TlsAlloc error";
        }
    }

    return TlsSetValue(g_tlsIndex, this);
}

IocContainer *IocContainer::getPinned()
{
    return static_cast<IocContainer *>(TlsGetValue(g_tlsIndex));
}

int IocContainer::getNextTypeId()
{
    return g_nextTypeId++;
}
