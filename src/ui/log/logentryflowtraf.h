#ifndef LOGENTRYFLOWTRAF_H
#define LOGENTRYFLOWTRAF_H

#include <common/fortconf.h>

#include "logentry.h"

class LogEntryFlowTraf : public LogEntry
{
public:
    explicit LogEntryFlowTraf(quint16 flowCount = 0, const char *flowsData = nullptr);

    FortLogType type() const override { return FORT_LOG_TYPE_FLOW_TRAF; }

    quint16 flowCount() const { return m_flowCount; }
    void setFlowCount(quint16 flowCount);

    const char *flowsData() const { return m_flowsData; }
    void setFlowsData(const char *flowsData);

    FORT_FLOW_STAT flowStat(int index) const;

private:
    quint16 m_flowCount = 0;
    const char *m_flowsData = nullptr;
};

#endif // LOGENTRYFLOWTRAF_H
