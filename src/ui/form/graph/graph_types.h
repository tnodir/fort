#ifndef GRAPH_TYPES_H
#define GRAPH_TYPES_H

#include <QCoreApplication>
#include <QStringList>

class IniGraph;

enum GraphColorType : qint8 {
    GraphColorBg = 0,
    GraphColorBorder,
    GraphColorIn,
    GraphColorOut,
    GraphColorTotal,
    GraphColorAxis,
    GraphColorTickLabel,
    GraphColorSpeedLabel,
    GraphColorGrid,
    GraphColorCount
};

enum GraphType : qint8 {
    GraphTypeColumn = 0,
    GraphTypeLine,
};

enum GraphSpeedPosition : qint8 {
    GraphSpeedTop = 0,
    GraphSpeedBottom,
    GraphSpeedLeft,
    GraphSpeedRight,
};

class GraphTypes
{
    Q_DECLARE_TR_FUNCTIONS(GraphTypes)

public:
    static GraphType graphTypeByIndex(int index)
    {
        return GraphType(qBound(int(GraphTypeColumn), index, int(GraphTypeLine)));
    }

    static QString graphName(const IniGraph &ini);

    static QStringList graphTypeNames();

    static GraphSpeedPosition speedPositionByIndex(int index)
    {
        return GraphSpeedPosition(qBound(int(GraphSpeedTop), index, int(GraphSpeedRight)));
    }

    static QStringList speedPositionNames();
};

#endif // GRAPH_TYPES_H
