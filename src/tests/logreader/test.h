#ifndef TEST_H
#define TEST_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(Device)
QT_FORWARD_DECLARE_CLASS(LogBuffer)

class Test : public QObject
{
    Q_OBJECT

private slots:
    void logRead();

private:
    void setConf(Device &device);
    void printLogs(LogBuffer &buf);
};

#endif // TEST_H
