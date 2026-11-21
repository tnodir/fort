#include "logentryflowtraf.h"

#include <common/fortlog.h>
#include <driver/drivercommon.h>

LogEntryFlowTraf::LogEntryFlowTraf(quint16 flowCount, const char *flowsData) :
    m_flowCount(flowCount), m_flowsData(flowsData)
{
}

void LogEntryFlowTraf::setFlowCount(quint16 flowCount)
{
    m_flowCount = flowCount;
}

void LogEntryFlowTraf::setFlowsData(const char *flowsData)
{
    m_flowsData = flowsData;
}

FORT_FLOW_STAT LogEntryFlowTraf::flowStat(int index) const
{
    Q_ASSERT(index >= 0 && index < m_flowCount);

    return DriverCommon::logFlowStatRead(m_flowsData + index * DriverCommon::logFlowStatSize());
}

bool LogEntryFlowTraf::isTickEnd() const
{
    return m_flowCount < FORT_LOG_FLOW_TRAF_BUFFER_FLOW_COUNT;
}
