#ifndef TASKWORKER_H
#define TASKWORKER_H

#include <QObject>

class TaskWorker : public QObject
{
    Q_OBJECT

public:
    explicit TaskWorker(QObject *parent = nullptr);

signals:
    void finished(bool success);

public slots:
    virtual void run() = 0;
    virtual void abort(bool success = false) = 0;
};

#endif // TASKWORKER_H
