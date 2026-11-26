#include "graph_types.h"

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
