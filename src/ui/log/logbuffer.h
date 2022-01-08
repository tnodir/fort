#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <QObject>
#include <QByteArray>

#include "logentry.h"

class LogEntryBlocked;
class LogEntryBlockedIp;
class LogEntryProcNew;
class LogEntryStatTraf;
class LogEntryTime;

class LogBuffer : public QObject
{
    Q_OBJECT

public:
    explicit LogBuffer(int bufferSize = 0, QObject *parent = nullptr);

    int top() const { return m_top; }
    int offset() const { return m_offset; }

    QByteArray &array() { return m_array; }

    FortLogType peekEntryType();

    void writeEntryBlocked(const LogEntryBlocked *logEntry);
    void readEntryBlocked(LogEntryBlocked *logEntry);

    void writeEntryBlockedIp(const LogEntryBlockedIp *logEntry);
    void readEntryBlockedIp(LogEntryBlockedIp *logEntry);

    void writeEntryProcNew(const LogEntryProcNew *logEntry);
    void readEntryProcNew(LogEntryProcNew *logEntry);

    void readEntryStatTraf(LogEntryStatTraf *logEntry);

    void writeEntryTime(const LogEntryTime *logEntry);
    void readEntryTime(LogEntryTime *logEntry);

public slots:
    void reset(int top = 0);

private:
    char *output();
    const char *input() const;

    void prepareFor(int len);

private:
    int m_top = 0;
    int m_offset = 0;

    QByteArray m_array;
};

#endif // LOGBUFFER_H
