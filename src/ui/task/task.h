#ifndef TASK_H
#define TASK_H

#include <QObject>

class FortManager;

class Task : public QObject
{
    Q_OBJECT

public:
    explicit Task(FortManager *fortManager,
                  QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }

signals:
    void finished();

public slots:
    virtual void run() = 0;
    virtual void cancel() = 0;

private:
    FortManager *m_fortManager;
};

#endif // TASK_H
