#include "ioccontainer.h"

#include <QDebug>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {

int g_tlsIndex = -1;
int g_nextTypeId = 0;

}

IocContainer::IocContainer(QObject *parent) : QObject(parent) { }

IocContainer::~IocContainer()
{
    tearDownAll();

    qDeleteAll(m_objects);

    if (g_tlsIndex != -1) {
        TlsFree(g_tlsIndex);
        g_tlsIndex = -1;
    }
}

void IocContainer::insertObject(int typeId, IocObject *obj)
{
    const int newSize = typeId + 1;
    if (newSize > m_objects.size()) {
        m_objects.resize(newSize);
    }

    m_objects[typeId] = obj;
}

IocObject *IocContainer::resolveObject(int typeId) const
{
    IocObject *obj = m_objects.at(typeId);
    Q_ASSERT(obj);
    return obj;
}

void IocContainer::setUpAll()
{
    for (IocObject *obj : qAsConst(m_objects)) {
        if (obj) {
            setUp(obj);
        }
    }
}

void IocContainer::tearDownAll()
{
    for (IocObject *obj : qAsConst(m_objects)) {
        if (obj) {
            obj->tearDown();
        }
    }
}

void IocContainer::setUp(IocObject *obj)
{
    if (!obj->wasSetUp()) {
        obj->setUp();
    }
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
