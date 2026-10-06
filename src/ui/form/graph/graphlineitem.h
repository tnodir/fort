#ifndef GRAPHLINEITEM_H
#define GRAPHLINEITEM_H

#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>

// Draws a smooth line of 2 device pixels through the points: it doesn't overshoot them
class GraphLineItem : public QGraphicsItem
{
public:
    explicit GraphLineItem(QGraphicsItem *parent = nullptr);

    const QColor &color() const { return m_color; }
    void setColor(const QColor &v);

    const QVector<QPointF> &points() const { return m_points; }
    void setPoints(const QVector<QPointF> &v);

    QRectF boundingRect() const override { return m_boundingRect; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget = nullptr) override;

private:
    QColor m_color;
    QRectF m_boundingRect;
    QPainterPath m_path;
    QVector<QPointF> m_points;
};

#endif // GRAPHLINEITEM_H
