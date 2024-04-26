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
    void setIsService(bool v) { m_isService = v; }

    bool hasService() const { return m_hasService; }
    void setHasService(bool v) { m_hasService = v; }

    bool forceDebug() const { return m_forceDebug; }
    void setForceDebug(bool v) { m_forceDebug = v; }

    bool debug() const { return m_debug || m_forceDebug; }
    void setDebug(bool v);

    bool console() const { return m_console; }
    void setConsole(bool v);

    void setPath(const QString &path);

    QString getFileTitle() const;

    static Logger *instance();

    static QString getDateString(const QString &format = "yyyy-MM-dd HH:mm:ss.zzz");

    static QString makeLogLine(
            Logger::LogLevel level, const QString &dateString, const QString &message);

public slots:
    void writeLog(const QString &dateString, const QString &logLine);

private:
    QString fileNamePrefix() const;
    QString fileNameSuffix() const;

    bool openLogFile();
    bool openDirFile();
    bool openLastFile();
    bool createNewFile();
    bool openFile(const QString &fileName);
    void closeFile();

    bool checkFileOpen(const QString &dateString);
    bool checkFileSize();

    void writeLogLine(const QString &logLine);

private:
    bool m_isService : 1 = false;
    bool m_hasService : 1 = false;
    bool m_forceDebug : 1 = false;
    bool m_debug : 1 = false;
    bool m_console : 1 = false;
    bool m_writing : 1 = false;

    QDir m_dir;
    QFile m_file;
};

#endif // LOGGER_H
