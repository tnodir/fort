#ifndef LOGENTRYPROCEXIT_H
#define LOGENTRYPROCEXIT_H

#include "logentryproc.h"

class LogEntryProcExit : public LogEntryProc
{
public:
    using LogEntryProc::LogEntryProc;

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_EXIT; }
};

#endif // LOGENTRYPROCEXIT_H
