#include "graphcacheditem.h"

#include <QPainter>

GraphCachedItem::GraphCachedItem(QGraphicsItem *parent) : QGraphicsItem(parent) { }

void GraphCachedItem::setCached(bool v)
{
    if (m_cached == v)
        return;

    m_cached = v;

    invalidateCache();
}

void GraphCachedItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!m_cached) {
        paintItem(painter);
        return;
    }

    if (boundingRect().isEmpty())
        return;

    const qreal dpr = painter->device()->devicePixelRatioF();

    if (m_cache.isNull() || m_cache.devicePixelRatio() != dpr) {
        updateCache(dpr);
    }

    painter->drawPixmap(m_cacheOrigin, m_cache);
}

void GraphCachedItem::invalidateCache()
{
    m_cache = QPixmap();

    update();
}

void GraphCachedItem::updateCache(qreal dpr)
{
    // Aligned to the device pixels: drawn without scaling, when scrolled by them
    const QRectF rect = boundingRect();
    const QRectF deviceRect(rect.topLeft() * dpr, rect.size() * dpr);
    const QRect alignedRect = deviceRect.toAlignedRect();

    m_cacheOrigin = QPointF(alignedRect.topLeft()) / dpr;

    m_cache = QPixmap(alignedRect.size());
    m_cache.setDevicePixelRatio(dpr);
    m_cache.fill(Qt::transparent);

    QPainter painter(&m_cache);
    painter.translate(-m_cacheOrigin);

    paintItem(&painter);
}
