#include "triggertimer.h"

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
