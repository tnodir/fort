#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <QObject>
#include <QByteArray>

#include "logentry.h"

class LogEntryBlocked;
class LogEntryProcNew;
class LogEntryStatTraf;

class LogBuffer : public QObject
{
    Q_OBJECT

public:
    explicit LogBuffer(int bufferSize = 0,
                       QObject *parent = nullptr);

    int top() const { return m_top; }
    int offset() const { return m_offset; }

    QByteArray &array() { return m_array; }

    LogEntry::LogType peekEntryType();

    void writeEntryBlocked(const LogEntryBlocked *logEntry);
    void readEntryBlocked(LogEntryBlocked *logEntry);

    void writeEntryProcNew(const LogEntryProcNew *logEntry);
    void readEntryProcNew(LogEntryProcNew *logEntry);

    void readEntryStatTraf(LogEntryStatTraf *logEntry);

signals:

public slots:
    void reset(int top = 0) {
        m_top = top;
        m_offset = 0;
    }

private:
    char *output();
    const char *input() const;

    void prepareFor(int len);

private:
    int m_top;
    int m_offset;

    QByteArray m_array;
};

#endif // LOGBUFFER_H
