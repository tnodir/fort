#ifndef TRIGGERTIMER_H
#define TRIGGERTIMER_H

#include <QTimer>

class TriggerTimer : public QTimer
{
    Q_OBJECT

public:
    explicit TriggerTimer(QObject *parent = nullptr);

    void startTrigger();
};

#endif // TRIGGERTIMER_H
