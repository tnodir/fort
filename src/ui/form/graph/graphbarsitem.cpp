#include "graphbarsitem.h"

#include <QPainter>

namespace {

inline constexpr double seamOpacity = 0.5; // of the outline

// A faint dotted line of a device pixel
QPen seamPen(const QBrush &outlineBrush)
{
    QColor color = outlineBrush.color();
    color.setAlphaF(color.alphaF() * seamOpacity);

    QPen pen(color, 0);
    pen.setDashPattern({ 2, 2 });
    pen.setCapStyle(Qt::FlatCap);

    return pen;
}

}

GraphBarsItem::GraphBarsItem(QGraphicsItem *parent) : QGraphicsItem(parent) { }

void GraphBarsItem::setBrush(const QBrush &v)
{
    if (m_brush == v)
        return;

    m_brush = v;

    update();
}

void GraphBarsItem::setOutlineBrush(const QBrush &v)
{
    if (m_outlineBrush == v)
        return;

    m_outlineBrush = v;
    m_seamPen = seamPen(v);

    updateRects();
}

void GraphBarsItem::setOutlineWidth(qreal v)
{
    if (m_outlineWidth == v)
        return;

    m_outlineWidth = v;

    updateRects();
}

void GraphBarsItem::setRects(const QVector<QRectF> &v, const QVector<QRectF> &neighbors)
{
    if (m_rects == v && m_neighbors == neighbors)
        return;

    m_rects = v;
    m_neighbors = neighbors;

    updateRects();
}

void GraphBarsItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(Qt::NoPen);

    if (!hasOutline()) {
        painter->setBrush(m_brush);
        painter->drawRects(m_rects.constData(), int(m_rects.size()));
        return;
    }

    // Fill the outline's color, then the inner rectangles over it
    painter->setBrush(m_outlineBrush);
    painter->drawRects(m_rects.constData(), int(m_rects.size()));

    painter->setBrush(m_brush);
    painter->drawRects(m_innerRects.constData(), int(m_innerRects.size()));

    painter->setPen(m_seamPen);
    painter->drawLines(m_seams.constData(), int(m_seams.size()));
}

bool GraphBarsItem::hasOutline() const
{
    return m_outlineWidth > 0 && m_outlineBrush.style() != Qt::NoBrush;
}

void GraphBarsItem::updateRects()
{
    prepareGeometryChange();

    m_innerRects.clear();
    m_seams.clear();

    if (hasOutline()) {
        setupInnerRects();
        setupNeighborBridges();
    }

    m_boundingRect = {};
    for (const QRectF &rect : std::as_const(m_rects)) {
        m_boundingRect |= rect;
    }

    // The bridges to the neighbors are out of the rectangles
    for (const QRectF &rect : std::as_const(m_innerRects)) {
        m_boundingRect |= rect;
    }

    update();
}

void GraphBarsItem::setupInnerRects()
{
    const qreal w = m_outlineWidth;
    const QRectF *prevRect = nullptr;

    m_innerRects.reserve(m_rects.size() * 2);

    for (const QRectF &rect : std::as_const(m_rects)) {
        // The thin bars are of the outline's color
        appendInnerRect(rect.adjusted(w, w, -w, -w));

        // Join the adjacent bars: the outline is around them
        if (prevRect) {
            appendBridge(*prevRect, rect);
        }

        prevRect = &rect;
    }
}

void GraphBarsItem::setupNeighborBridges()
{
    for (const QRectF &neighbor : std::as_const(m_neighbors)) {
        for (const QRectF &rect : std::as_const(m_rects)) {
            appendBridge(neighbor, rect);
            appendBridge(rect, neighbor);
        }
    }
}

void GraphBarsItem::appendBridge(const QRectF &left, const QRectF &right)
{
    const qreal w = m_outlineWidth;

    if (qAbs(right.left() - left.right()) > w / 2)
        return; // not adjacent

    // Over the outlines between the bars, inside their common height
    const QPointF topLeft(left.right() - w, qMax(left.top(), right.top()) + w);
    const QPointF bottomRight(right.left() + w, qMin(left.bottom(), right.bottom()) - w);
    const QRectF bridge(topLeft, bottomRight);

    if (!bridge.isValid())
        return;

    m_innerRects.append(bridge);

    // As in NetTraffic: the seconds are still visible
    const qreal x = right.left();
    m_seams.append(QLineF(x, bridge.top(), x, bridge.bottom()));
}

void GraphBarsItem::appendInnerRect(const QRectF &rect)
{
    if (rect.isValid()) {
        m_innerRects.append(rect);
    }
}
