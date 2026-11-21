#include "logentryflowtraf.h"

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
