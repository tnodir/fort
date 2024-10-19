#include "taskeditinfo.h"

#include "taskinfo.h"

static_assert(sizeof(TaskEditInfo) == sizeof(quint64), "TaskEditInfo size mismatch");

TaskEditInfo::TaskEditInfo(quint64 v) : m_value(v) { }

void TaskEditInfo::resetToDefault()
{
    setEnabled(false);
    setRunOnStartup(false);
    setDelayStartup(false);
    setMaxRetries(0);
    setRetrySeconds(0);
    setIntervalHours(TaskDefaultIntervalHours);
}
