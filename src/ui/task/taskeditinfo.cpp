#include "taskeditinfo.h"

TaskEditInfo::TaskEditInfo(bool enabled, bool runOnStartup, quint16 intervalHours) :
    m_enabled(enabled), m_runOnStartup(runOnStartup), m_intervalHours(intervalHours)
{
}

TaskEditInfo::TaskEditInfo(quint32 v) : m_value(v) { }
