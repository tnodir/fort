#ifndef TRIGGERTIMER_H
#define TRIGGERTIMER_H

#include <QTimer>

class TriggerTimer : public QTimer
{
    Q_OBJECT

public:
    constexpr static int DefaultInterval = 200;

    explicit TriggerTimer(QObject *parent = nullptr);
    explicit TriggerTimer(int interval, QObject *parent = nullptr);

public slots:
    void startTrigger();
};

#endif // TRIGGERTIMER_H
