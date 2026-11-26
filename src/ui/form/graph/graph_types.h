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

enum GraphFontType : qint8 { GraphFontTickLabel = 0, GraphFontSpeedLabel, GraphFontCount };

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

enum GraphAxisLine : qint8 {
    GraphAxisLineLeft = 0,
    GraphAxisLineTop,
    GraphAxisLineRight,
    GraphAxisLineBottom,
    GraphAxisLineCount
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

    // The bit of the line in the mask of the visible ones
    static constexpr int axisLineMask(GraphAxisLine line) { return 1 << line; }

    static QStringList axisLineNames();
};

#endif // GRAPH_TYPES_H
