#include "graph_types.h"

#include <user/inigraph.h>

QString GraphTypes::graphName(const IniGraph &ini)
{
    // The main graph's name is fixed
    return (ini.graphId() == 0) ? tr("Traffic Graph") : ini.name();
}

QStringList GraphTypes::graphTypeNames()
{
    // Sync with GraphType
    return { tr("Column"), tr("Line") };
}

QStringList GraphTypes::speedPositionNames()
{
    // Sync with GraphSpeedPosition
    return { tr("Top"), tr("Bottom"), tr("Left"), tr("Right") };
}
