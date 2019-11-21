#ifndef TEST_H
#define TEST_H

#include <QObject>

class Test : public QObject
{
    Q_OBJECT

private slots:
    void confWriteRead();
    void checkPeriod();
    void checkEnvManager();
};

#endif // TEST_H
