#include "triggertimer.h"

TriggerTimer::TriggerTimer(QObject *parent) : TriggerTimer(/*interval=*/DefaultInterval, parent) { }

TriggerTimer::TriggerTimer(int interval, QObject *parent) : QTimer(parent)
{
    setSingleShot(true);
    setInterval(interval);
}

void TriggerTimer::startTrigger()
{
    if (!isActive()) {
        start();
    }
}
