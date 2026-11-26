#ifndef GRAPH_TYPES_H
#define GRAPH_TYPES_H

#include <QCoreApplication>
#include <QStringList>

enum GraphColorType : qint8 {
    GraphColorBg = 0,
    GraphColorBorder,
    GraphColorIn,
    GraphColorOut,
    GraphColorTotal,
    GraphColorAxis,
    GraphColorTickLabel,
    GraphColorLabel,
    GraphColorGrid,
    GraphColorCount
};

enum GraphType : qint8 {
    GraphTypeColumn = 0,
    GraphTypeLine,
};

class GraphTypes
{
    Q_DECLARE_TR_FUNCTIONS(GraphTypes)

public:
    static GraphType graphTypeByIndex(int index)
    {
        return GraphType(qBound(int(GraphTypeColumn), index, int(GraphTypeLine)));
    }

    static QStringList graphTypeNames();
};

#endif // GRAPH_TYPES_H
