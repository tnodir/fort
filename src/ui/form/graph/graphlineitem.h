#ifndef GRAPHLINEITEM_H
#define GRAPHLINEITEM_H

#include <QBrush>
#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QPixmap>

// Draws a smooth line of 2 device pixels through the points: it doesn't overshoot them.
// The area below the line may be filled by a gradient of its color.
// They are cached in a pixmap: the scrolled item isn't repainted.
class GraphLineItem : public QGraphicsItem
{
public:
    explicit GraphLineItem(QGraphicsItem *parent = nullptr);

    const QColor &color() const { return m_color; }
    void setColor(const QColor &v);

    bool fillVisible() const { return m_fillVisible; }
    void setFillVisible(bool v);

    const QVector<QPointF> &points() const { return m_points; }
    void setPoints(const QVector<QPointF> &v);

    QRectF boundingRect() const override { return m_boundingRect; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget = nullptr) override;

private:
    void updateFill();
    void setupFill();

    void updateCache(qreal dpr);

    void paintFill(QPainter *painter);
    void paintLine(QPainter *painter);

private:
    bool m_fillVisible = false;

    QColor m_color;
    QBrush m_fillBrush;
    QPointF m_cacheOrigin;
    QRectF m_boundingRect;
    QPainterPath m_path;
    QPainterPath m_fillPath;
    QPixmap m_cache;
    QVector<QPointF> m_points;
};

#endif // GRAPHLINEITEM_H
