#include "crashhandler.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>

#include <client/windows/handler/exception_handler.h>

Q_DECLARE_LOGGING_CATEGORY(CLOG_CRASH_HANDLER)
Q_LOGGING_CATEGORY(CLOG_CRASH_HANDLER, "breakpad")

#define logDebug()   qCDebug(CLOG_CRASH_HANDLER, )
#define logWarning() qCWarning(CLOG_CRASH_HANDLER, )

namespace {

void processMinidump(const CrashHandler *crashHandler, const QString &dumpFilePath)
{
    const QString namePrefix = crashHandler->fileNamePrefix();
    const QString nameSuffix = crashHandler->fileNameSuffix();

    const QString newFileName = (namePrefix.isEmpty() ? QString() : namePrefix)
            + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss_zzz")
            + (nameSuffix.isEmpty() ? QString() : nameSuffix);

    const QFileInfo fi(dumpFilePath);
    QDir dumpDir = fi.dir();

    // Rename mini-dump file name
    const QString newFilePath = dumpDir.rename(fi.fileName(), newFileName)
            ? dumpDir.filePath(newFileName)
            : dumpFilePath;

    logWarning() << "Crash dump created:" << newFilePath;
}

bool minidumpCallback(const wchar_t *dumpPath, const wchar_t *minidumpId, void *context,
        EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion, bool succeeded)
{
    Q_UNUSED(exinfo);
    Q_UNUSED(assertion);

    if (succeeded) {
        const QString dumpFilePath = QString::fromStdWString(dumpPath) + '/'
                + QString::fromStdWString(minidumpId) + ".dmp";

        processMinidump(static_cast<CrashHandler *>(context), dumpFilePath);
    }
    return succeeded;
}

}

CrashHandler::CrashHandler() = default;

CrashHandler::~CrashHandler()
{
    uninstall();
}

void CrashHandler::install(const QString &dumpPath)
{
    Q_ASSERT(!m_exceptionHandler);

    m_exceptionHandler = new google_breakpad::ExceptionHandler(dumpPath.toStdWString(), nullptr,
            &minidumpCallback, this, google_breakpad::ExceptionHandler::HANDLER_ALL);
}

void CrashHandler::uninstall()
{
    if (m_exceptionHandler) {
        delete m_exceptionHandler;
        m_exceptionHandler = nullptr;
    }
}
