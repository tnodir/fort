#ifndef LOGENTRYPROCKILL_H
#define LOGENTRYPROCKILL_H

#include "logentryproc.h"

class LogEntryProcKill : public LogEntryProc
{
public:
    using LogEntryProc::LogEntryProc;

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_KILL; }
};

#endif // LOGENTRYPROCKILL_H
