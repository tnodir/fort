#include "osutil.h"

#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QUrl>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "processinfo.h"

QString OsUtil::pidToPath(quint32 pid, bool isKernelPath)
{
    const ProcessInfo pi(pid);
    return pi.path(isKernelPath);
}

bool OsUtil::openUrlExternally(const QUrl &url)
{
    return QDesktopServices::openUrl(url);
}

bool OsUtil::openFolder(const QString &filePath)
{
    const QString nativePath = QDir::toNativeSeparators(filePath);

    return QProcess::execute("explorer.exe", { "/select,", nativePath }) == 0;
}

bool OsUtil::openUrlOrFolder(const QString &path)
{
    const QUrl url = QUrl::fromUserInput(path);

    if (url.isLocalFile()) {
        const QFileInfo fi(path);
        if (!fi.isDir()) {
            return OsUtil::openFolder(path);
        }
        // else open the folder's content
    }

    return openUrlExternally(url);
}

void *OsUtil::createMutex(const char *name, bool &isSingleInstance)
{
    void *h = CreateMutexA(nullptr, FALSE, name);
    isSingleInstance = h && GetLastError() != ERROR_ALREADY_EXISTS;
    return h;
}

void OsUtil::closeMutex(void *mutexHandle)
{
    CloseHandle(mutexHandle);
}

quint32 OsUtil::lastErrorCode()
{
    return GetLastError();
}

QString OsUtil::errorMessage(quint32 errorCode)
{
    LPWSTR buf = nullptr;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorCode, 0, (LPWSTR) &buf, 0, nullptr);

    if (!buf) {
        return QString("System Error %1").arg(errorCode);
    }

    const QString text = QString::fromUtf16((const char16_t *) buf).trimmed();
    LocalFree(buf);
    return text;
}

qint32 OsUtil::getTickCount()
{
    return qint32(GetTickCount());
}

bool OsUtil::isUserAdmin()
{
    SID_IDENTIFIER_AUTHORITY idAuth = SECURITY_NT_AUTHORITY;
    PSID adminGroup;
    BOOL res = AllocateAndInitializeSid(&idAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup);
    if (res) {
        if (!CheckTokenMembership(nullptr, adminGroup, &res)) {
            res = false;
        }
        FreeSid(adminGroup);
    }

    return res;
}
