#include "triggertimer.h"

TriggerTimer::TriggerTimer(QObject *parent) : QTimer(parent)
{
    setSingleShot(true);
    setInterval(200);
}

void TriggerTimer::startTrigger()
{
    if (!isActive()) {
        start();
    }
}
