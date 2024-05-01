#include "osutil.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QLoggingCategory>
#include <QProcess>
#include <QUrl>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <lmcons.h>

#include "fileutil.h"
#include "processinfo.h"

namespace {

const QLoggingCategory LC("util.osUtil");

BOOL WINAPI consoleCtrlHandler(DWORD /*ctrlType*/)
{
    OsUtil::quit("console control");

    Sleep(100); // Let the process exit gracefully
    return TRUE;
}

}

QString OsUtil::pidToPath(quint32 pid, bool isKernelPath)
{
    const ProcessInfo pi(pid);
    return pi.path(isKernelPath);
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

    return QDesktopServices::openUrl(url);
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

QString OsUtil::userName()
{
    wchar_t buf[UNLEN + 1];
    DWORD len = UNLEN + 1;
    if (GetUserNameW(buf, &len)) {
        return QString::fromWCharArray(buf, int(len) - 1); // skip terminationg null char.
    }
    return QString();
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

bool OsUtil::beep(BeepType type)
{
    return MessageBeep(type);
}

void OsUtil::showConsole(bool visible)
{
    if (visible) {
        if (AllocConsole()) {
            SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);

            // Disable close button of console window
            const HMENU hMenu = GetSystemMenu(GetConsoleWindow(), false);
            DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
        }
    } else {
        FreeConsole();
    }
}

void OsUtil::writeToConsole(const QString &line)
{
    const HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD nw;
    WriteConsoleW(stdoutHandle, line.utf16(), line.size(), &nw, nullptr);
}

bool OsUtil::setCurrentThreadName(const QString &name)
{
    if (name.isEmpty())
        return true;

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const HRESULT hr = SetThreadDescription(GetCurrentThread(), (PCWSTR) name.utf16());
    return SUCCEEDED(hr);
#else
    return false;
#endif
}

void OsUtil::setThreadIsBusy(bool on)
{
    // Works correct only on Windows 11+
    SetThreadExecutionState(ES_CONTINUOUS | (on ? ES_SYSTEM_REQUIRED : 0));
}

bool OsUtil::allowOtherForegroundWindows()
{
    return AllowSetForegroundWindow(ASFW_ANY);
}

bool OsUtil::registerAppRestart()
{
    return SUCCEEDED(
            RegisterApplicationRestart(L"--restarted", RESTART_NO_CRASH | RESTART_NO_REBOOT));
}

void OsUtil::restartClient()
{
    const QFileInfo fi(QCoreApplication::applicationFilePath());

    const auto scriptPath = QLatin1String("cmd.exe");
    const auto command = QString("timeout /t 2 >NUL & start %1 --restarted").arg(fi.fileName());

    const QStringList args = { "/c", command };

    qCDebug(LC) << "restartClient:" << scriptPath << args;

    QProcess::startDetached(scriptPath, args, /*workingDirectory=*/fi.path());

    quit("required client restart");
}

void OsUtil::restart()
{
    const QString appFilePath = QCoreApplication::applicationFilePath();

    QStringList args = QCoreApplication::arguments();
    args.removeFirst(); // remove a program path

    qCDebug(LC) << "restart:" << appFilePath << args;

    qApp->connect(qApp, &QObject::destroyed, [=] { QProcess::startDetached(appFilePath, args); });

    quit("required restart");
}

void OsUtil::quit(const QString &reason)
{
    qCDebug(LC) << "Quit due" << reason;

    QCoreApplication::quit();
}
