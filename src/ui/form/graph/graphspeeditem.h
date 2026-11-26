#ifndef GRAPHSPEEDITEM_H
#define GRAPHSPEEDITEM_H

#include <QGraphicsRectItem>

class QGraphicsPathItem;
class QGraphicsSimpleTextItem;

// The text speed: the in and out texts after their arrows, in a line or in two rows
class GraphSpeedItem : public QGraphicsRectItem
{
public:
    explicit GraphSpeedItem(QGraphicsItem *parent = nullptr);

    void setText(const QString &inText, const QString &outText);

    // The arrows are colored by their bars
    void setColors(const QColor &bgColor, const QColor &inColor, const QColor &outColor,
            const QColor &textColor);

    void setTextFont(const QFont &font);

    // By the widest speed text: the graph doesn't move with the speed
    qreal rowsWidth() const;

    // Lays out the texts and sets the box's rectangle by their size
    QSizeF layout(bool rows);

private:
    void setupArrows(const QFont &font);

    QSizeF layoutLine();
    QSizeF layoutRows();

private:
    qreal m_spacing = 0; // between the in and out texts

    QGraphicsPathItem *m_inArrow = nullptr;
    QGraphicsSimpleTextItem *m_inText = nullptr;
    QGraphicsPathItem *m_outArrow = nullptr;
    QGraphicsSimpleTextItem *m_outText = nullptr;
};

#endif // GRAPHSPEEDITEM_H
