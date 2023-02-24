#ifndef TRIGGERTIMER_H
#define TRIGGERTIMER_H

#include <QTimer>

class TriggerTimer : public QTimer
{
    Q_OBJECT

public:
    constexpr static int DefaultInterval = 200;

    explicit TriggerTimer(int interval = DefaultInterval, QObject *parent = nullptr);

    void startTrigger();
};

#endif // TRIGGERTIMER_H
