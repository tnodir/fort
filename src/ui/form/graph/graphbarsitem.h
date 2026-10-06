#ifndef GRAPHBARSITEM_H
#define GRAPHBARSITEM_H

#include <QBrush>
#include <QGraphicsItem>

// Draws the bars' rectangles: it's much faster than filling a QPainterPath of them
class GraphBarsItem : public QGraphicsItem
{
public:
    explicit GraphBarsItem(QGraphicsItem *parent = nullptr);

    const QBrush &brush() const { return m_brush; }
    void setBrush(const QBrush &v);

    // A device pixel's outline inside the rectangles
    const QBrush &outlineBrush() const { return m_outlineBrush; }
    void setOutlineBrush(const QBrush &v);

    const QVector<QRectF> &rects() const { return m_rects; }
    void setRects(const QVector<QRectF> &v);

    QRectF boundingRect() const override { return m_boundingRect; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget = nullptr) override;

private:
    QVector<QRectF> innerRects(qreal lineWidth) const;

private:
    QBrush m_brush;
    QBrush m_outlineBrush;
    QRectF m_boundingRect;
    QVector<QRectF> m_rects;
};

#endif // GRAPHBARSITEM_H
