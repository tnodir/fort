#include "dirinfo.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <util/fileutil.h>

DirInfo::DirInfo() { }

DirInfo::~DirInfo()
{
    close();
}

void DirInfo::setPath(const QString &path)
{
    m_path = FileUtil::toNativeSeparators(path);
}

bool DirInfo::open()
{
    close();

    m_handle = CreateFileW((LPCWSTR) path().utf16(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    return isOpen();
}

void DirInfo::close()
{
    if (m_handle == DIR_INVALID_HANDLE)
        return;

    CloseHandle(m_handle);

    m_handle = DIR_INVALID_HANDLE;
}

bool DirInfo::checkIsValid()
{
    if (m_handle == DIR_INVALID_HANDLE)
        return false;

    if (GetFileType(m_handle) == FILE_TYPE_UNKNOWN) {
        close();
        return false;
    }

    return true;
}
