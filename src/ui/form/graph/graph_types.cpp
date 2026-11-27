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

QStringList GraphTypes::sideNames()
{
    // Sync with GraphSide
    return { tr("Top"), tr("Bottom"), tr("Left"), tr("Right") };
}

QStringList GraphTypes::themeNames()
{
    // Sync with GraphTheme
    return { tr("System"), tr("Light"), tr("Dark") };
}
