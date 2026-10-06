#ifndef GRAPHBARSITEM_H
#define GRAPHBARSITEM_H

#include <QBrush>
#include <QGraphicsItem>
#include <QPen>

// Draws the bars' rectangles: it's much faster than filling a QPainterPath of them
class GraphBarsItem : public QGraphicsItem
{
public:
    explicit GraphBarsItem(QGraphicsItem *parent = nullptr);

    const QBrush &brush() const { return m_brush; }
    void setBrush(const QBrush &v);

    // The outline is inside the rectangles, around the adjacent ones
    const QBrush &outlineBrush() const { return m_outlineBrush; }
    void setOutlineBrush(const QBrush &v);

    qreal outlineWidth() const { return m_outlineWidth; }
    void setOutlineWidth(qreal v);

    // The neighbors are drawn by another item: the outline is around them too
    const QVector<QRectF> &rects() const { return m_rects; }
    void setRects(const QVector<QRectF> &v, const QVector<QRectF> &neighbors = {});

    QRectF boundingRect() const override { return m_boundingRect; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget = nullptr) override;

private:
    bool hasOutline() const;

    void updateRects();
    void setupInnerRects();
    void setupNeighborBridges();

    void appendBridge(const QRectF &left, const QRectF &right);
    void appendInnerRect(const QRectF &rect);

private:
    qreal m_outlineWidth = 0;

    QBrush m_brush;
    QBrush m_outlineBrush;
    QPen m_seamPen;
    QRectF m_boundingRect;

    QVector<QRectF> m_rects;
    QVector<QRectF> m_neighbors;
    QVector<QRectF> m_innerRects; // and the bridges between the adjacent rectangles
    QVector<QLineF> m_seams; // between the adjacent rectangles
};

#endif // GRAPHBARSITEM_H
