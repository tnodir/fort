#include "threadstorage.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {
const QLoggingCategory LC("util.threadStorage");
}

ThreadStorage::ThreadStorage()
{
    createTlsIndex();
}

ThreadStorage::~ThreadStorage()
{
    deleteTlsIndex();
}

void *ThreadStorage::value() const
{
    return TlsGetValue(m_tlsIndex);
}

bool ThreadStorage::setValue(void *v)
{
    return TlsSetValue(m_tlsIndex, v);
}

void ThreadStorage::createTlsIndex()
{
    if (m_tlsIndex != BadTlsIndex)
        return;

    m_tlsIndex = TlsAlloc();

    if (m_tlsIndex == BadTlsIndex) {
        qCCritical(LC) << "TlsAlloc error";
    }
}

void ThreadStorage::deleteTlsIndex()
{
    if (m_tlsIndex == BadTlsIndex)
        return;

    TlsFree(m_tlsIndex);

    m_tlsIndex = BadTlsIndex;
}
