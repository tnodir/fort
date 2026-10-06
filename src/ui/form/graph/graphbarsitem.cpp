#include "graphbarsitem.h"

#include <QPainter>

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

    update();
}

void GraphBarsItem::setRects(const QVector<QRectF> &v)
{
    if (m_rects == v)
        return;

    prepareGeometryChange();

    m_rects = v;

    m_boundingRect = {};
    for (const QRectF &rect : v) {
        m_boundingRect |= rect;
    }

    update();
}

void GraphBarsItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(Qt::NoPen);

    if (m_outlineBrush.style() == Qt::NoBrush) {
        painter->setBrush(m_brush);
        painter->drawRects(m_rects.constData(), int(m_rects.size()));
        return;
    }

    // Fill the outline's color, then the inner rectangles over it
    const QVector<QRectF> inner = innerRects(1 / painter->device()->devicePixelRatioF());

    painter->setBrush(m_outlineBrush);
    painter->drawRects(m_rects.constData(), int(m_rects.size()));

    painter->setBrush(m_brush);
    painter->drawRects(inner.constData(), int(inner.size()));
}

QVector<QRectF> GraphBarsItem::innerRects(qreal lineWidth) const
{
    QVector<QRectF> rects;
    rects.reserve(m_rects.size());

    for (const QRectF &rect : m_rects) {
        const QRectF inner = rect.adjusted(lineWidth, lineWidth, -lineWidth, -lineWidth);

        // The thin bars are of the outline's color
        if (inner.isValid()) {
            rects.append(inner);
        }
    }

    return rects;
}
