#ifndef LOGGER_H
#define LOGGER_H

#include <QDir>
#include <QFile>
#include <QLoggingCategory>

class Logger : public QObject
{
    Q_OBJECT

protected:
    explicit Logger(QObject *parent = nullptr);

public:
    enum LogLevel { Info = 0, Warning, Error };
    Q_ENUM(LogLevel)

    bool active() const { return m_active; }
    void setActive(bool active);

    bool debug() const { return m_debug; }
    void setDebug(bool debug);

    bool console() const { return m_console; }
    void setConsole(bool console);

    void setPath(const QString &path);

    static Logger *instance();

public slots:
    void writeLog(const QString &message, Logger::LogLevel level = Info);
    void writeLogList(
            const QString &message, const QStringList &list, Logger::LogLevel level = Info);

signals:

private:
    bool openLogFile();
    void closeLogFile();

    void writeLogLine(Logger::LogLevel level, const QString &dateString, const QString &message);

    void checkLogFiles();

    static void messageHandler(
            QtMsgType type, const QMessageLogContext &context, const QString &message);

private:
    bool m_active : 1;
    bool m_debug : 1;
    bool m_console : 1;
    bool m_writing : 1;

    QDir m_dir;
    QFile m_file;
};

#endif // LOGGER_H
