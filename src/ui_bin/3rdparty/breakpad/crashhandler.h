#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <QString>

namespace google_breakpad {
class ExceptionHandler;
}

class CrashHandler
{
public:
    explicit CrashHandler();
    ~CrashHandler();

    QString fileNamePrefix() const { return m_fileNamePrefix; }
    void setFileNamePrefix(const QString &v) { m_fileNamePrefix = v; }

    QString fileNameSuffix() const { return m_fileNameSuffix; }
    void setFileNameSuffix(const QString &v) { m_fileNameSuffix = v; }

    void install(const QString &dumpPath);
    void uninstall();

private:
    QString m_fileNamePrefix;
    QString m_fileNameSuffix;

    google_breakpad::ExceptionHandler *m_exceptionHandler = nullptr;
};

#endif // CRASHHANDLER_H
