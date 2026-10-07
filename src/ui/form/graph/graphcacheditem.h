#ifndef GRAPHCACHEDITEM_H
#define GRAPHCACHEDITEM_H

#include <QGraphicsItem>
#include <QPixmap>

// Caches the painting in a pixmap aligned to the device pixels:
// the item, scrolled by the device pixels, only draws the pixmap
class GraphCachedItem : public QGraphicsItem
{
public:
    explicit GraphCachedItem(QGraphicsItem *parent = nullptr);

    bool cached() const { return m_cached; }
    void setCached(bool v);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget = nullptr) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    // Call on the painting's change
    void invalidateCache();

    virtual void paintItem(QPainter *painter) = 0;

private:
    void updateCache(qreal dpr);
    void preparePixmap(qreal dpr);

private:
    bool m_cached : 1 = false;
    bool m_cacheValid : 1 = false;

    QSize m_cacheSize; // device pixels: the used part of the pixmap
    QPointF m_cacheOrigin;
    QPixmap m_cache;
};

#endif // GRAPHCACHEDITEM_H
