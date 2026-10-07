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

    if (!m_cacheValid || m_cache.devicePixelRatio() != dpr) {
        updateCache(dpr);
    }

    // The used part of the pixmap
    const QRectF sourceRect(QPointF(0, 0), QSizeF(m_cacheSize));

    painter->drawPixmap(QRectF(m_cacheOrigin, sourceRect.size() / dpr), m_cache, sourceRect);
}

QVariant GraphCachedItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // Free the hidden item's cache
    if (change == ItemVisibleHasChanged && !value.toBool()) {
        m_cache = QPixmap();
        m_cacheValid = false;
    }

    return QGraphicsItem::itemChange(change, value);
}

void GraphCachedItem::invalidateCache()
{
    m_cacheValid = false;

    update();
}

void GraphCachedItem::updateCache(qreal dpr)
{
    // Aligned to the device pixels: drawn without scaling, when scrolled by them
    const QRectF rect = boundingRect();
    const QRectF deviceRect(rect.topLeft() * dpr, rect.size() * dpr);
    const QRect alignedRect = deviceRect.toAlignedRect();

    m_cacheOrigin = QPointF(alignedRect.topLeft()) / dpr;
    m_cacheSize = alignedRect.size();

    preparePixmap(dpr);

    QPainter painter(&m_cache);

    // Clear the used part of the reused pixmap
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(QRectF(QPointF(0, 0), QSizeF(m_cacheSize) / dpr), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter.translate(-m_cacheOrigin);

    paintItem(&painter);

    m_cacheValid = true;
}

void GraphCachedItem::preparePixmap(qreal dpr)
{
    // Reuse the big enough pixmap: the item's size is changed by its values
    if (m_cache.devicePixelRatio() == dpr && m_cacheSize.boundedTo(m_cache.size()) == m_cacheSize)
        return;

    m_cache = QPixmap(m_cacheSize.expandedTo(m_cache.size()));
    m_cache.setDevicePixelRatio(dpr);
    m_cache.fill(Qt::transparent); // with the alpha channel
}
