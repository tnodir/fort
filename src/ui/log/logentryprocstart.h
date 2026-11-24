#ifndef LOGENTRYPROCSTART_H
#define LOGENTRYPROCSTART_H

#include "logentryprocpath.h"

class LogEntryProcStart : public LogEntryProcPath
{
public:
    using LogEntryProcPath::LogEntryProcPath;

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_START; }

    quint32 parentPid() const { return m_parentPid; }
    void setParentPid(quint32 v) { m_parentPid = v; }

    qint64 createTime() const { return m_createTime; }
    void setCreateTime(qint64 v) { m_createTime = v; }

private:
    quint32 m_parentPid = 0;
    qint64 m_createTime = 0;
};

#endif // LOGENTRYPROCSTART_H
