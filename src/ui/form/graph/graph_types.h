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

// The values of the enums below are stored in the ini: don't reorder them

enum GraphType : qint8 {
    GraphTypeColumn = 0,
    GraphTypeLine,
};

// A side of the graph
enum GraphSide : qint8 {
    GraphSideTop = 0,
    GraphSideBottom,
    GraphSideLeft,
    GraphSideRight,
    GraphSideCount
};

// The colors of a graph: the light or the dark ones
enum GraphTheme : qint8 {
    GraphThemeSystem = 0,
    GraphThemeLight,
    GraphThemeDark,
};

class GraphTypes
{
    Q_DECLARE_TR_FUNCTIONS(GraphTypes)

public:
    static GraphType graphTypeByIndex(int index)
    {
        return enumByIndex(index, GraphTypeColumn, GraphTypeLine);
    }

    static QString graphName(const IniGraph &ini);

    static QStringList graphTypeNames();

    static GraphSide speedPositionByIndex(int index)
    {
        return enumByIndex(index, GraphSideTop, GraphSideRight);
    }

    static QStringList sideNames();

    static GraphTheme themeByIndex(int index)
    {
        return enumByIndex(index, GraphThemeSystem, GraphThemeDark);
    }

    static QStringList themeNames();

    // The bit of the side's axis line in the mask of the visible ones
    static constexpr int axisLineMask(GraphSide side) { return 1 << side; }

private:
    // The index is bounded by the enum's first and last values
    template<typename T>
    static T enumByIndex(int index, T first, T last)
    {
        return T(qBound(int(first), index, int(last)));
    }
};

#endif // GRAPH_TYPES_H
