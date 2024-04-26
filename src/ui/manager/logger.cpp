#include "logger.h"

#include <QLoggingCategory>

#include <fort_version.h>

#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/osutil.h>

namespace {

const QLoggingCategory LC("logger");

constexpr int LOGGER_FILE_MAX_SIZE = 1024 * 1024;
constexpr int LOGGER_KEEP_FILES = 9;

QtMessageHandler g_oldMessageHandler = nullptr;

Logger::LogLevel levelByMsgType(QtMsgType type)
{
    switch (type) {
    case QtWarningMsg:
        return Logger::Warning;
    case QtCriticalMsg:
    case QtFatalMsg:
        return Logger::Error;
    default:
        return Logger::Info;
    }
}

void processMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const Logger::LogLevel level = levelByMsgType(type);

    Logger *logger = Logger::instance();

    const bool isLogToFile = (level != Logger::Info || logger->debug());
    const bool isLogConsole = logger->console();

    if (!(isLogToFile || isLogConsole))
        return;

    const bool isDefaultCategory = !context.category || !strcmp(context.category, "default");
    const QString text =
            isDefaultCategory ? message : QLatin1String(context.category) + ": " + message;

    const auto dateString = Logger::getDateString();
    const auto logLine = Logger::makeLogLine(level, dateString, text);

    // Write only errors to log file
    if (isLogToFile) {
        logger->writeLog(dateString, logLine);
    }

    // Additionally write to console if needed
    if (isLogConsole) {
        OsUtil::writeToConsole(logLine);
    }
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    processMessage(type, context, message);

    if (g_oldMessageHandler) {
        g_oldMessageHandler(type, context, message);
    }
}

}

Logger::Logger(QObject *parent) : QObject(parent)
{
    g_oldMessageHandler = qInstallMessageHandler(messageHandler);
}

void Logger::setDebug(bool v)
{
    if (m_debug == v)
        return;

    m_debug = v;

    if (!m_debug) {
        if (forceDebug())
            return;

        closeFile();
    }

    QLoggingCategory::setFilterRules(debug() ? QString() : "*.debug=false");
}

void Logger::setConsole(bool v)
{
    if (m_console == v)
        return;

    m_console = v;

    if (!isService()) {
        OsUtil::showConsole(console());
    }
}

void Logger::setPath(const QString &path)
{
    m_dir.setPath(path);
}

QString Logger::getFileTitle() const
{
    return QLatin1String(APP_NAME) + ' ' + APP_VERSION_STR + APP_VERSION_BUILD_STR
            + (isService() ? " Service" : (hasService() ? " Client" : QString()));
}

Logger *Logger::instance()
{
    static Logger *g_instanceLogger = nullptr;

    if (!g_instanceLogger) {
        g_instanceLogger = new Logger();
    }
    return g_instanceLogger;
}

QString Logger::getDateString(const QString &format)
{
    return DateUtil::now().toString(format);
}

QString Logger::makeLogLine(LogLevel level, const QString &dateString, const QString &message)
{
    static const char *const g_levelChars = ".WE";

    return dateString + ' ' + g_levelChars[int(level)] + ' ' + message + '\n';
}

QString Logger::fileNamePrefix() const
{
    return QLatin1String("log_fort_")
            + (isService() ? "svc_" : (hasService() ? "clt_" : QString()));
}

QString Logger::fileNameSuffix() const
{
    return QLatin1String(".txt");
}

bool Logger::openLogFile()
{
    if (openDirFile())
        return true;

    m_dir.setPath(FileUtil::pathSlash(FileUtil::appConfigLocation()) + "logs/");
    if (openDirFile())
        return true;

    m_dir.setPath(FileUtil::pathSlash(FileUtil::tempLocation()) + APP_NAME);
    if (openDirFile())
        return true;

    qCWarning(LC) << "Cannot open log file:" << m_file.fileName() << m_file.errorString();

    return false;
}

bool Logger::openDirFile()
{
    m_dir.mkpath("./");

    return openLastFile() || createNewFile();
}

bool Logger::openLastFile()
{
    const auto fileNames = FileUtil::getFileNames(m_dir, fileNamePrefix(), fileNameSuffix());
    if (fileNames.isEmpty())
        return false;

    const auto &fileName = fileNames.first();
    if (!openFile(fileName))
        return false;

    if (!checkFileSize()) {
        closeFile();
        return false;
    }

    writeLogLine("\n");

    return true;
}

bool Logger::createNewFile()
{
    const QString fileName =
            fileNamePrefix() + getDateString("yyyy-MM-dd_HH-mm-ss_zzz") + fileNameSuffix();

    return openFile(fileName);
}

bool Logger::openFile(const QString &fileName)
{
    m_file.setFileName(m_dir.filePath(fileName));

    return m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void Logger::closeFile()
{
    m_file.close();
}

bool Logger::checkFileOpen(const QString &dateString)
{
    // Create file when required to avoid empty files
    if (m_file.isOpen())
        return true;

    FileUtil::removeOldFiles(m_dir, fileNamePrefix(), fileNameSuffix(), LOGGER_KEEP_FILES);

    if (!openLogFile())
        return false;

    // Write file header
    writeLogLine(makeLogLine(Info, dateString, getFileTitle()));

    return true;
}

bool Logger::checkFileSize()
{
    if (m_file.size() < LOGGER_FILE_MAX_SIZE)
        return true;

    closeFile(); // too big file

    return false;
}

void Logger::writeLogLine(const QString &logLine)
{
    m_file.write(logLine.toUtf8());
    m_file.flush();
}

void Logger::writeLog(const QString &dateString, const QString &logLine)
{
    if (m_writing)
        return; // avoid recursive calls

    m_writing = true;

    if (checkFileOpen(dateString)) {
        writeLogLine(logLine);
        checkFileSize();
    }

    m_writing = false;
}
