#ifndef TEST_H
#define TEST_H

#include <QObject>

class Test : public QObject
{
    Q_OBJECT

private slots:
    void ip4Text();
    void ip4Ranges();
    void taskTasix();
};

#endif // TEST_H
