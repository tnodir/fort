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
    enum LogLevel { Info = 0, Warning, Error, LevelCount };
    Q_ENUM(LogLevel)

    bool active() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    bool console() const { return m_console; }
    void setConsole(bool console) { m_console = console; }

    void setPath(const QString &path);

    static Logger *instance();

    static void setupLogging(bool enabled, bool debug, bool console);

public slots:
    void writeLog(const QString &message, LogLevel level = Info);
    void writeLogList(const QString &message, const QStringList &list,
                      LogLevel level = Info);

signals:

private:
    bool openLogFile();
    void closeLogFile();

    void writeLogLine(Logger::LogLevel level, const QString &dateString,
                      const QString &message);

    void checkLogFiles();

    static void messageHandler(QtMsgType type,
                               const QMessageLogContext &context,
                               const QString &message);

private:
    uint m_active   : 1;
    uint m_console  : 1;
    uint m_writing  : 1;

    QDir m_dir;
    QFile m_file;
};

#endif // LOGGER_H
