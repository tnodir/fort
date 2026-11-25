#ifndef GRAPH_TYPES_H
#define GRAPH_TYPES_H

#include <QtGlobal>

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

#endif // GRAPH_TYPES_H
