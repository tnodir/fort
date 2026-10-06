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
    painter->setBrush(m_brush);
    painter->drawRects(m_rects.constData(), int(m_rects.size()));
}
