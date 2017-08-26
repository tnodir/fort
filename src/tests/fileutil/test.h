#ifndef TEST_H
#define TEST_H

#include <QObject>

class Test : public QObject
{
    Q_OBJECT

private slots:
    void paths();
    void process();
};

#endif // TEST_H
