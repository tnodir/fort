#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <QObject>
#include <QByteArray>

#include "logentry.h"

class LogEntryBlocked;

class LogBuffer : public QObject
{
    Q_OBJECT

public:
    explicit LogBuffer(int bufferSize = 0,
                       QObject *parent = nullptr);

    int top() const { return m_top; }
    int offset() const { return m_offset; }

    QByteArray &array() { return m_array; }

    LogEntry::LogType readType();

signals:

public slots:
    void reset(int top = 0) {
        m_top = top;
        m_offset = 0;
    }

    int write(const LogEntryBlocked *logEntry);
    int read(LogEntryBlocked *logEntry);

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
