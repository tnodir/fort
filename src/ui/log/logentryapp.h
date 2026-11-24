#ifndef LOGENTRYAPP_H
#define LOGENTRYAPP_H

#include "logentryprocpath.h"

class LogEntryApp : public LogEntryProcPath
{
public:
    using LogEntryProcPath::LogEntryProcPath;

    FortLogType type() const override { return FORT_LOG_TYPE_APP; }

    bool blocked() const { return m_blocked; }
    void setBlocked(bool v) { m_blocked = v; }

    bool alerted() const { return m_alerted; }
    void setAlerted(bool v) { m_alerted = v; }

private:
    bool m_blocked : 1 = true;
    bool m_alerted : 1 = true;
};

#endif // LOGENTRYAPP_H
