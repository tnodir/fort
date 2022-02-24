#ifndef LOGGER_H
#define LOGGER_H

#include <QDir>
#include <QFile>

class Logger : public QObject
{
    Q_OBJECT

protected:
    explicit Logger(QObject *parent = nullptr);

public:
    enum LogLevel { Info = 0, Warning, Error };
    Q_ENUM(LogLevel)

    bool isService() const { return m_isService; }
    void setIsService(bool v);

    bool debug() const { return m_debug; }
    void setDebug(bool v);

    bool console() const { return m_console; }
    void setConsole(bool v);

    void setPath(const QString &path);

    static Logger *instance();

public slots:
    void writeLog(const QString &message, Logger::LogLevel level = Info);
    void writeLogList(
            const QString &message, const QStringList &list, Logger::LogLevel level = Info);

private:
    QString fileNamePrefix() const;
    QString fileNameSuffix() const;

    bool openLogFile();
    bool tryOpenLogFile(const QDir &dir, const QString &fileName);
    void closeLogFile();

    void writeLogLine(Logger::LogLevel level, const QString &dateString, const QString &message);

private:
    bool m_isService : 1;
    bool m_debug : 1;
    bool m_console : 1;
    bool m_writing : 1;

    QDir m_dir;
    QFile m_file;
};

#endif // LOGGER_H
