#include "device.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>
#include <winioctl.h>

Device::Device(QObject *parent) :
    QObject(parent), m_handle(INVALID_HANDLE_VALUE), m_buffer(sizeof(OVERLAPPED))
{
}

Device::~Device()
{
    close();
}

bool Device::isOpened() const
{
    return (m_handle != INVALID_HANDLE_VALUE);
}

bool Device::open(const QString &filePath, quint32 flags)
{
    const DWORD access = GENERIC_READ | ((flags & ReadWrite) != 0 ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD creation = OPEN_EXISTING;
    const DWORD attr = FILE_ATTRIBUTE_NORMAL | (SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION)
            | ((flags & Overlapped) != 0 ? FILE_FLAG_OVERLAPPED : 0);

    m_flags = flags;
    m_handle = CreateFileW(
            (LPCWSTR) filePath.utf16(), access, share, nullptr, creation, attr, nullptr);

    return m_handle != INVALID_HANDLE_VALUE;
}

bool Device::close()
{
    bool res = false;
    if (m_handle != INVALID_HANDLE_VALUE) {
        res = CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
    return res;
}

bool Device::cancelIo()
{
    return CancelIoEx(m_handle, nullptr);
}

bool Device::ioctl(quint32 code, char *in, int inSize, char *out, int outSize, qsizetype *retSize)
{
    LPOVERLAPPED overlapped = isOverlapped() ? (LPOVERLAPPED) m_buffer.data() : nullptr;

    DWORD size = 0; // lpBytesReturned cannot be nullptr on Win7
    const bool ok = DeviceIoControl(m_handle, code, in, inSize, out, outSize, &size, overlapped);

    if (retSize) {
        *retSize = size;
    }

    return ok;
}

void Device::initOverlapped(void *eventHandle)
{
    LPOVERLAPPED overlapped = (LPOVERLAPPED) m_buffer.data();
    ZeroMemory(overlapped, sizeof(OVERLAPPED));
    overlapped->hEvent = eventHandle;
}
