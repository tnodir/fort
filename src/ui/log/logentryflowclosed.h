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

private:
    quint64 m_flowId = 0;
};

#endif // LOGENTRYFLOWCLOSED_H
