#ifndef TRIGGERTIMER_H
#define TRIGGERTIMER_H

#include <QTimer>

class TriggerTimer : public QTimer
{
    Q_OBJECT

public:
    constexpr static int DefaultInterval = 100;
    constexpr static int SearchInterval = 150; // to apply a search text being typed

    explicit TriggerTimer(QObject *parent = nullptr);
    explicit TriggerTimer(int interval, QObject *parent = nullptr);

public slots:
    void startTrigger();
};

#endif // TRIGGERTIMER_H
