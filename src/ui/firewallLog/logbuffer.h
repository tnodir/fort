#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <QObject>
#include <QByteArray>

class LogEntry;

class LogBuffer : public QObject
{
    Q_OBJECT

public:
    explicit LogBuffer(QObject *parent = nullptr);

signals:

public slots:
    void write(const LogEntry &logEntry);
    bool read(LogEntry &logEntry);

private:
    void prepareFor(int len);

private:
    int m_top;
    int m_offset;

    QByteArray m_array;
};

#endif // LOGBUFFER_H
