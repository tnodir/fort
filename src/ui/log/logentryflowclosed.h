#ifndef LOGENTRYFLOWCLOSED_H
#define LOGENTRYFLOWCLOSED_H

#include "logentry.h"

class LogEntryFlowClosed : public LogEntry
{
public:
    explicit LogEntryFlowClosed(quint64 flowId = 0);

    FortLogType type() const override { return FORT_LOG_TYPE_FLOW_CLOSED; }

    quint64 flowId() const { return m_flowId; }
    void setFlowId(quint64 flowId) { m_flowId = flowId; }

    quint32 inBytes() const { return m_inBytes; }
    void setInBytes(quint32 v) { m_inBytes = v; }

    quint32 outBytes() const { return m_outBytes; }
    void setOutBytes(quint32 v) { m_outBytes = v; }

private:
    quint32 m_inBytes = 0;
    quint32 m_outBytes = 0;
    quint64 m_flowId = 0;
};

#endif // LOGENTRYFLOWCLOSED_H
